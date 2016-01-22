#include <tchar.h>
#include <wchar.h>
#include <string>

#include "util.h"
#include "file.h"
#include "globalresources.h"
#include "game.h"
#include "gameplay.h"

namespace Parameter {
	void help();
	// TODO: char **argv version is necessary.
	bool parse(int argc, _TCHAR **argv);
}

namespace Parameter {
	void help() {
		wprintf(L"SimpleBmxPlayer\n================\n-- How to use -- \n\n"
			L"argument: (bmx file) (options ...)\n"
			L"options:"
			L"-noimage: don't load image files (ignore image channel)\n"
			L"-ms: start from n-th measure\n"
			L"-repeat: repeat bms for n-times\n"
			L"keys:\n"
			L"default key config is -\n(1P) LS Z S X D C F V (2P) M K , L . ; / RS\nyou can change it by changing preset files.\n"
			L"Press F5 to reload BMS file only.\n"
			L"Press F6 to reload BMS Resource file only.\n"
			L"Press - to move -5 measures.\n"
			L"Press + to move +5 measures.\n"
			L"Press Up/Down to change speed.\n"
			L"Press Right/Left to change lane.\n"
			L"Press Shift+Right/Left to change lift.\n");
	}

	bool parse(int argc, _TCHAR **argv) {
		char *buf = new char[10240];
		if (argc <= 1)
			return false;

		// set BMS path from argument
		ENCODING::wchar_to_utf8(argv[1], buf, 10240);
		STRPOOL->Set("Bmspath", buf);

		delete buf;
		return true;
	}
}

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
	 * pool is an part of game system
	 * MUST be initalized very first
	 */
	PoolHelper::InitalizeAll();

	/*
	 * Parse parameter for specific option
	 * if failed, exit.
	 */
	if (!Parameter::parse(argc, argv)) {
		Parameter::help();
		PoolHelper::ReleaseAll();
		return -1;
	}

	/* 
	 * Game basic initalization
	 * (our start scene is playing, cause it's simple bmx player..?)
	 */
	Game::Initialize();
	Game::ChangeScene(GamePlay::SCENE);
	
	/* 
	 * Game main loop started
	 */
	Game::MainLoop();

	/*
	 * Okay, game end, release everything
	 */
	Game::Release();
	PoolHelper::ReleaseAll();
	SDL_Quit();
	return 0;
}