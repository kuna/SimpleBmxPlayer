#include <tchar.h>
#include <wchar.h>
#include <string>

#include "game.h"

int _tmain(int argc, _TCHAR **argv) {
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