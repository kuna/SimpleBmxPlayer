#include <tchar.h>
#include <wchar.h>
#include <string>

#include "game.h"

#ifdef _WIN32
#include <Windows.h>
#endif
int _tmain(int argc, _TCHAR **argv) {
#ifdef _WIN32
	SetCurrentDirectory(IO::get_filedir(argv[0]).c_str());
#endif

	if (!Game::Parameter::parse(argc, argv)) {
		Game::Parameter::help();
		return -1;
	}

	/* initalization */
	Game::Init();
	
	// Let's render something ...
	Game::Start();
	Game::MainLoop();

	// release everything
	Game::Release();
	SDL_Quit();

	// end
	return 0;
}