#include <tchar.h>
#include <wchar.h>
#include <string>

#include "game.h"
#include "util.h"
#include "file.h"

int _tmain(int argc, _TCHAR **argv) {
	/*
	 * Relative Directory could be changed when *.bmx files are given in argv
	 * So, we need to reset CurrentDirectory in Win32
	 * (Don't know this behaviour is either occured in Linux/Mac ...)
	 */
	char utf8path[1024];
	ENCODING::wchar_to_utf8(IO::get_filedir(argv[0]).c_str(), utf8path, 1024);
	FileHelper::PushBasePath(utf8path);

	/*
	 * Parse parameter for specific option
	 */
	if (!Game::Parameter::parse(argc, argv)) {
		Game::Parameter::help();
		return -1;
	}

	/* Game basic initalization */
	Game::Init();
	
	/* Game Start! */
#if 1
	// this routine is inactivated until Lua Test is finished.
	Game::Start();
	Game::MainLoop();
#endif

	/* Okay, game end, release everything */
	Game::Release();
	SDL_Quit();
	return 0;
}