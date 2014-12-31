#include "analysis_cut_by_pattern.hpp"
#include "utils.hpp"
#include "cut_section.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "analysis_cut_by_mid_air.hpp"
#include "analysis_cut_by_multi_rail.hpp"


struct CutSection : public udtCutSection
{
	// qsort isn't guaranteed to be stable, so we work around that.
	int Order;
};

static int SortByStartTimeAscending(const void* aPtr, const void* bPtr)
{
	const u64 a = ((CutSection*)aPtr)->StartTimeMs;
	const u64 b = ((CutSection*)bPtr)->StartTimeMs;

	return (int)(a - b);
}

static int StableSortByGameStateIndexAscending(const void* aPtr, const void* bPtr)
{
	const CutSection& a = *(CutSection*)aPtr;
	const CutSection& b = *(CutSection*)bPtr;

	const int byGameState = a.GameStateIndex - b.GameStateIndex;
	const int byPreviousOrder = a.Order - b.Order;

	return byGameState != 0 ? byGameState : byPreviousOrder;
}


udtCutByPatternPlugIn::udtCutByPatternPlugIn(udtVMLinearAllocator& analyzerAllocator, const udtCutByPatternArg& info)
	: _analyzerAllocatorScope(analyzerAllocator)
	, _info(info)
	, _trackedPlayerIndex(S32_MIN)
{
	_tempAllocator.Init(BIG_INFO_STRING, UDT_MEMORY_PAGE_SIZE);
}

udtCutByPatternAnalyzerBase* udtCutByPatternPlugIn::CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo)
{
	if(extraInfo == NULL)
	{
		return NULL;
	}

#define UDT_CUT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) case udtPatternType::Enum: analyzer = _analyzerAllocatorScope.NewObject<AnalyzerType>(); break;
	udtCutByPatternAnalyzerBase* analyzer = NULL;
	switch(patternType)
	{
		UDT_CUT_PATTERN_LIST(UDT_CUT_PATTERN_ITEM)
		default: return NULL;
	}
#undef UDT_CUT_PATTERN_ITEM

	if(analyzer != NULL)
	{
		analyzer->PlugIn = this;
		analyzer->ExtraInfo = extraInfo;
		_analyzers.Add(analyzer);
		_analyzerTypes.Add(patternType);
	}

	return analyzer;
}

udtCutByPatternAnalyzerBase* udtCutByPatternPlugIn::GetAnalyzer(udtPatternType::Id patternType)
{
	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		if(_analyzerTypes[i] == patternType)
		{
			return _analyzers[i];
		}
	}

	return NULL;
}

void udtCutByPatternPlugIn::ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
{
	_trackedPlayerIndex = S32_MIN;
	if(_info.PlayerIndex >= 0 && _info.PlayerIndex < 64)
	{
		_trackedPlayerIndex = _info.PlayerIndex;
	}
	else if(_info.PlayerIndex == (s32)udtPlayerIndex::DemoTaker)
	{
		_trackedPlayerIndex = info.ClientNum;
	}
	else if(!StringIsNullOrEmpty(_info.PlayerName))
	{
		const s32 firstPlayerCsIdx = parser._protocol == udtProtocol::Dm68 ? CS_PLAYERS_68 : CS_PLAYERS_73p;
		for(s32 i = 0; i < MAX_CLIENTS; ++i)
		{
			const char* const playerName = GetPlayerName(parser, firstPlayerCsIdx + i);
			if(!StringIsNullOrEmpty(playerName) && StringEquals(playerName, _info.PlayerName))
			{
				_trackedPlayerIndex = i;
				break;
			}
		}
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessGamestateMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
{
	if(_info.PlayerIndex == (s32)udtPlayerIndex::FirstPersonPlayer)
	{
		idPlayerStateBase* const ps = GetPlayerState(info.Snapshot, parser._protocol);
		if(ps != NULL)
		{
			_trackedPlayerIndex = ps->clientNum;
		}
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessSnapshotMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::TrackPlayerFromCommandMessage(udtBaseParser& parser)
{
	if(_trackedPlayerIndex != S32_MIN || StringIsNullOrEmpty(_info.PlayerName))
	{
		return;
	}

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const int tokenCount = tokenizer.argc();
	if(strcmp(tokenizer.argv(0), "cs") != 0 || tokenCount == 3)
	{
		return;
	}

	int csIndex = -1;
	if(!StringParseInt(csIndex, tokenizer.argv(1)))
	{
		return;
	}

	const s32 firstPlayerCsIdx = parser._protocol == udtProtocol::Dm68 ? CS_PLAYERS_68 : CS_PLAYERS_73p;
	const s32 playerIndex = csIndex - firstPlayerCsIdx;
	if(playerIndex < 0 || playerIndex >= MAX_CLIENTS)
	{
		return;
	}

	const char* const playerName = GetPlayerName(parser, csIndex);
	if(!StringIsNullOrEmpty(playerName) && StringEquals(playerName, _info.PlayerName))
	{
		_trackedPlayerIndex = playerIndex;
	}
}

void udtCutByPatternPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	TrackPlayerFromCommandMessage(parser);

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessCommandMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::FinishAnalysis()
{
	if(_analyzers.GetSize() == 0)
	{
		return;
	}

	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->FinishAnalysis();
	}

	// If we only have 1 analyzer, we don't need to do any sorting.
	if(_analyzers.GetSize() == 1)
	{
		MergeRanges(CutSections, _analyzers[0]->CutSections);
		return;
	}

	//
	// Create a list with all the cut sections.
	//
	udtVMArray<CutSection> tempCutSections;
	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		udtCutByPatternAnalyzerBase* const analyzer = _analyzers[i];
		for(u32 j = 0, cutCount = analyzer->CutSections.GetSize(); j < cutCount; ++j)
		{
			const udtCutSection cut = _analyzers[i]->CutSections[j];
			CutSection newCut;
			newCut.GameStateIndex = cut.GameStateIndex;
			newCut.StartTimeMs = cut.StartTimeMs;
			newCut.EndTimeMs = cut.EndTimeMs;
			tempCutSections.Add(newCut);
		}
	}

	//
	// Apply sorting pass #1.
	//
	const u32 cutCount = tempCutSections.GetSize();
	qsort(tempCutSections.GetStartAddress(), (size_t)cutCount, sizeof(CutSection), &SortByStartTimeAscending);

	//
	// Apply sorting pass #2, which must be stable with respect to 
	// the sorting of pass #1.
	//
	for(u32 i = 0; i < cutCount; ++i)
	{
		tempCutSections[i].Order = (int)i;
	}
	qsort(tempCutSections.GetStartAddress(), (size_t)cutCount, sizeof(CutSection), &StableSortByGameStateIndexAscending);

	//
	// Create a new list with the sorted data using the final data format
	// and merge the sections.
	//
	udtVMArray<udtCutSection> cutSections;
	for(u32 i = 0; i < cutCount; ++i)
	{
		const CutSection cut = tempCutSections[i];
		udtCutSection newCut;
		newCut.GameStateIndex = cut.GameStateIndex;
		newCut.StartTimeMs = cut.StartTimeMs;
		newCut.EndTimeMs = cut.EndTimeMs;
		cutSections.Add(newCut);
	}

	MergeRanges(CutSections, cutSections);
}

s32 udtCutByPatternPlugIn::GetTrackedPlayerIndex() const
{
	return _trackedPlayerIndex;
}

const char* udtCutByPatternPlugIn::GetPlayerName(udtBaseParser& parser, s32 csIdx)
{
	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(csIdx);
	if(cs == NULL)
	{
		return NULL;
	}

	char* playerName = NULL;
	if(!ParseConfigStringValueString(playerName, _tempAllocator, "n", cs->String))
	{
		return NULL;
	}

	playerName = Q_CleanStr(playerName);
	StringMakeLowerCase(playerName);

	return playerName;
}