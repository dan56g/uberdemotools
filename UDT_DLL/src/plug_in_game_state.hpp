#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "api.h"


struct udtParserPlugInGameState : udtBaseParserPlugIn
{
public:
	udtParserPlugInGameState();
	~udtParserPlugInGameState();

	void  ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser);
	void  ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser);
	void  ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void  FinishAnalysis();
	u32   GetElementCount() const { return _gameStates.GetSize(); }
	void* GetFirstElementAddress() { return _gameStates.GetStartAddress(); }

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInGameState);

private:
	void ClearMatch();
	void ClearGameState();
	void AddCurrentMatchIfValid();
	void AddCurrentGameState();
	void ProcessCpmaGameInfo(const char* commandString, udtBaseParser& parser);
	void ProcessCpmaTwTs(s32 tw, s32 ts, s32 serverTimeMs);
	void ProcessQlServerInfo(const char* commandString, udtBaseParser& parser);

private:
	// "gamename" in cs 0.
	struct udtGameType
	{
		enum Id
		{
			BaseQ3, // baseq3, osp, etc
			CPMA,
			QL
		};
	};

	// "g_gameState" in cs 0.
	struct udtGameStateQL
	{
		enum Id
		{
			Invalid,
			PreGame,
			CountDown,
			InProgress
		};
	};

	udtVMArray<udtParseDataGameState> _gameStates;
	udtVMArray<udtMatchInfo> _matches;

	udtParseDataGameState _currentGameState;
	udtMatchInfo _currentMatch;

	udtGameType::Id _gameType;
	udtGameStateQL::Id _gameStateQL;

	bool _firstGameState;
	bool _nextSnapshotIsWarmUpEnd;
};