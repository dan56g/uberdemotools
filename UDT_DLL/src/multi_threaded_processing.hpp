#pragma once


#include "uberdemotools.h"
#include "array.hpp"
#include "api_helpers.hpp"
#include "timer.hpp"


struct udtParsingSharedData
{
	const char** FilePaths;
	u64* FileSizes;
	const udtParseArg* ParseInfo;
	const udtMultiParseArg* MultiParseInfo;
	const void* JobSpecificInfo;
	u32 JobType; // Of type udtParsingJobType::Id.
};

struct udtParsingThreadData
{
	u64 TotalByteCount;
	udtParsingSharedData* Shared;
	udtParserContext* Context;
	u32 FirstFileIndex;
	u32 FileCount;
	f32 Progress;
	bool Finished;
	bool Stop;
	bool Result;
};

struct udtDemoThreadAllocator
{
	udtDemoThreadAllocator();

	// Returns true if more than 1 thread should be launched.
	bool Process(const char** filePaths, u32 fileCount, u32 maxThreadCount);

	udtVMArrayWithAlloc<const char*> FilePaths;
	udtVMArrayWithAlloc<u64> FileSizes;
	udtVMArrayWithAlloc<u32> InputIndices;
	udtVMArrayWithAlloc<udtParsingThreadData> Threads;
};

struct udtMultiThreadedParsing
{
	bool Process(udtTimer& jobTimer,
				 udtParserContext* contexts, 
                 udtDemoThreadAllocator& threadInfo, 
				 const udtParseArg* parseInfo, 
				 const udtMultiParseArg* multiParseInfo,
				 udtParsingJobType::Id jobType,
				 const void* jobSpecificInfo);
};
