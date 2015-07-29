#include "analysis_obituaries.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


void udtObituariesAnalyzer::InitAllocators(u32 demoCount, udtVMLinearAllocator& finalAllocator, udtVMLinearAllocator& tempAllocator)
{
	if(_enableNameAllocation)
	{
		_playerNamesAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	}

	finalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	_tempAllocator = &tempAllocator;
	Obituaries.SetAllocator(finalAllocator);
}

void udtObituariesAnalyzer::ResetForNextDemo()
{
	_gameStateIndex = -1;

	for(u32 i = 0; i < 64; ++i)
	{
		_playerTeams[i] = -1;
	}
}

void udtObituariesAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		udtObituaryEvent eventInfo;
		if(!IsObituaryEvent(eventInfo, *arg.Entities[i].Entity, parser._inProtocol))
		{
			continue;
		}

		const s32 targetTeamIdx = _playerTeams[eventInfo.TargetIndex];
		const s32 attackerTeamIdx = (eventInfo.AttackerIndex == -1) ? -1 : _playerTeams[eventInfo.AttackerIndex];
		const char* const targetName = AllocatePlayerName(parser, eventInfo.TargetIndex);
		const char* const attackerName = AllocatePlayerName(parser, eventInfo.AttackerIndex);

		udtParseDataObituary info;
		info.TargetTeamIdx = targetTeamIdx;
		info.AttackerTeamIdx = attackerTeamIdx;
		info.MeanOfDeath = eventInfo.MeanOfDeath;
		info.GameStateIndex = parser._inGameStateIndex;
		info.ServerTimeMs = arg.Snapshot->serverTime;
		info.TargetIdx = eventInfo.TargetIndex;
		info.AttackerIdx = eventInfo.AttackerIndex;
		info.TargetName = targetName;
		info.AttackerName = attackerName;
		info.MeanOfDeathName = GetUDTModName(eventInfo.MeanOfDeath);
		Obituaries.Add(info);
	}
}

const char* udtObituariesAnalyzer::AllocatePlayerName(udtBaseParser& parser, s32 playerIdx)
{
	if(!_enableNameAllocation)
	{
		return NULL;
	}

	if(playerIdx == -1)
	{
		return AllocateString(_playerNamesAllocator, "world");
	}

	const s32 firstPlayerCsIdx = idConfigStringIndex::FirstPlayer(parser._inProtocol);
	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(firstPlayerCsIdx + playerIdx);
	if(cs == NULL)
	{
		return NULL;
	}

	udtString playerName;
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);
	if(!ParseConfigStringValueString(playerName, *_tempAllocator, "n", cs->String))
	{
		return NULL;
	}

	udtString::CleanUp(playerName);

	return AllocateString(_playerNamesAllocator, playerName.String);
}

void udtObituariesAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const s32 csFirstPlayerIdx = idConfigStringIndex::FirstPlayer(parser._inProtocol);
	for(s32 i = 0; i < 64; ++i)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(csFirstPlayerIdx + i);
		if(cs != NULL)
		{
			udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);
			ParseConfigStringValueInt(_playerTeams[i], *_tempAllocator, "t", cs->String);
		}
	}
}

void udtObituariesAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const int tokenCount = tokenizer.GetArgCount();
	if(strcmp(tokenizer.GetArgString(0), "cs") != 0 || tokenCount != 3)
	{
		return;
	}

	s32 csIndex = -1;
	if(!StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const s32 csFirstPlayerIdx = idConfigStringIndex::FirstPlayer(parser._inProtocol);
	const s32 playerIdx = csIndex - csFirstPlayerIdx;
	if(playerIdx < 0 || playerIdx >= 64)
	{
		return;
	}

	udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(csIndex);
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);
	ParseConfigStringValueInt(_playerTeams[playerIdx], *_tempAllocator, "t", cs->String);
}
