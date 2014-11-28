#include "parser.hpp"
#include "file_stream.hpp"
#include "shared.hpp"
#include "utils.hpp"
#include "analysis_splitter.hpp"

#include <stdio.h>


static bool RunDemoSplitter(const char* filePath)
{
	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.MessageCb = &CallbackConsoleMessage;

	udtParserContext* const context = udtCreateContext(NULL);
	if(context == NULL)
	{
		return false;
	}

	const bool result = udtSplitDemoFile(context, &info, filePath) == udtErrorCode::None;
	udtDestroyContext(context);

	return result;
}

static void PrintHelp()
{
	printf("???? help for UDT_splitter ????\n");
	printf("Syntax: UDT_splitter demo_path\n");
}

int main(int argc, char** argv)
{
	ResetCurrentDirectory(argv[0]);

	if(argc < 2)
	{
		printf("Not enough arguments.\n");
		PrintHelp();
		return 1;
	}

	if(!udtFileStream::Exists(argv[1]))
	{
		printf("Invalid file path.\n");
		PrintHelp();
		return 2;
	}

	const bool success = RunDemoSplitter(argv[1]);

	return success ? 0 : 666;
}