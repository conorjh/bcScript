#ifndef BCVM_H
#define BCVM_H
#include <string>
#include "..\bcScript\bcscript.h"

namespace bc
{
	namespace bcvm
	{
		struct bcvmData
		{
			std::string runFile;
			bool runFileIsByteCode,disableConsoleInput;
			vm::bcVM vm;
		};

		int parseCmdLineArg(const char * args[], int p_i);
		int parseCmdLineArg_Run(const char * args[], int p_i);
		int parseCmdLineArg_DisableConsoleInput(const char * args[], int p_i);
		void execCmdLine();

		void run();
		int consoleLoop();
		std::string consoleGetInput();
		int consoleParseCmd(std::string);

		bool isRunning();

		bcvmData* getData();
	}
}

#endif