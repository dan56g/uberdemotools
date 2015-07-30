#include "analysis_cut_by_chat.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"


udtCutByChatAnalyzer::udtCutByChatAnalyzer() 
	: _cutSections(1 << 16)
{
}

udtCutByChatAnalyzer::~udtCutByChatAnalyzer()
{
}

void udtCutByChatAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& commandInfo, udtBaseParser& parser)
{
	udtContext& context = *parser._context;
	CommandLineTokenizer& tokenizer = context.Tokenizer;
	tokenizer.Tokenize(commandInfo.String);
	if(tokenizer.GetArgCount() != 2 || !udtString::Equals(tokenizer.GetArg(0), "chat"))
	{
		return;
	}

	const udtCutByChatArg& extraInfo = GetExtraInfo<udtCutByChatArg>();

	bool match = false;
	for(u32 i = 0; i < extraInfo.RuleCount; ++i)
	{
		udtVMScopedStackAllocator tempAllocatorScopeGuard(parser._tempAllocator);
		if(StringMatchesCutByChatRule(tokenizer.GetArg(1), extraInfo.Rules[i], parser._tempAllocator, parser._inProtocol))
		{
			match = true;
			break;
		}
	}

	if(!match)
	{
		return;
	}

	const udtCutByPatternArg& patternInfo = PlugIn->GetInfo();
	const s32 startTimeMs = parser._inServerTime - (s32)patternInfo.StartOffsetSec * 1000;
	const s32 endTimeMs = parser._inServerTime + (s32)patternInfo.EndOffsetSec * 1000;

	udtCutSection cutSection;
	cutSection.VeryShortDesc = "chat";
	cutSection.GameStateIndex = parser._inGameStateIndex;
	cutSection.StartTimeMs = startTimeMs;
	cutSection.EndTimeMs = endTimeMs;
	_cutSections.Add(cutSection);
}

void udtCutByChatAnalyzer::StartAnalysis()
{
	_cutSections.Clear();
}

void udtCutByChatAnalyzer::FinishAnalysis()
{
	MergeRanges(CutSections, _cutSections);
}
