#include <tchar.h>
#include <wchar.h>
#include <string>

#include "util.h"
#include "file.h"
#include "game.h"
#include "logger.h"
#include "Profile.h"
#include "Setting.h"


namespace {
	void help() {
		printf("SimpleBmxPlayer\n================\n-- How to use -- \n\n"
			"argument: (bmx file) (options ...)\n"
			"- .bmx(bms;bml;bme;pms) file must put in\n"
			"- course play available (test1.bms;test2.bms;test3.bms)\n"
			"- archive file available (ex: ./ex.zip/test.bms)\n"
			"<options>\n"
			"-bgaoff: don't load image files (ignore image channel)\n"
			"-replay: show replay file (if no replay file, it won't turn on)\n"
			"-auto: autoplayed by DJ\n"
			"-op__: set op for player (RANDOM ... etc; only support int value)\n"
			"\n"
			"-g_: set gauge (0: GROOVE, 1: EASY, 2: HARD, 3: EXHARD, 4: HAZARD)\n"
			"-n______: set user profile to play (default: NONAME)\n"
			"-s_: start from n-th measure\n"
			"-e_: end(cut) at n-th measure\n"
			"-r_: repeat bms for n-times\n"
			"-rate__: song rate (1.0 is normal, lower is faster; decimal)\n"
			"-pace__: pacemaker percent (decimal)\n"
			"\n"
			"<keys>\n"
			"default key config is -\n(1P) LS Z S X D C F V\n(2P) RS M K , L . ; /\nyou can change it by changing preset files.\n"
			"[Up/Down]		change speed.\n"
			"[Right/Left]	change sudden.\n"
			"[F12]			float speed toggle\n"
			"[Start+WB/BB]	change speed\n"
			"[Start+SC]		(if float on) change float speed.\n"
			"[Start+SC]		(if sudden on) change sudden.\n"
			"[Start+SC]		(if sudden off) change lift.\n"
			"[Start+VEFX]	float speed toggle\n"
			"[Start double]	toggle sudden.\n"
			"\n"
			"(WB: white button; generally call 1,3,5,7 buttons.)\n"
			"(BB: blue? black? btton; generally calls 2, 4, 6 buttons)\n"
			"(SC: scratch; LShift on keyboard)\n"
			);
	}

	bool parse(int argc, char **argv) {
		// bmspath : must required option
		// if not specified, return false.
		if (argc <= 1) {
			help();
			return false;
		}

		/*
		* Relative Directory could be changed when *.bmx files are given in argv
		* So, we need to reset CurrentDirectory in Win32
		* (Don't know this behaviour is either occured in Linux/Mac ...)
		*/
		RString basepath = get_filedir(argv[0]);
		FILEMANAGER->PushBasePath(basepath.c_str());

		/* Fill parameter */
		PARAMETER.targetpath = argv[1];
		for (int i = 2; i < argc; i++) {
			PARAMETER.argv.push_back(argv[i]);
		}
		return true;
	}

	bool parse(int argc, _TCHAR **argv) {
		char *buf = new char[10240];
		if (argc <= 1)
			return false;

		// convert all argument to utf8 and pass it to parse(utf8)
		char **argv_utf8 = new char*[argc];
		for (int i = 0; i < argc; i++) {
			RString buf = WStringToRString(argv[i]);
			argv_utf8[i] = new char[buf.size() + 1];
			strcpy(argv_utf8[i], buf.c_str());
		}
		bool r = parse(argc, argv_utf8);
		for (int i = 0; i < argc; i++)
			delete[] argv_utf8[i];

		delete[] argv_utf8;
		return r;
	}
}



#ifdef _WIN32
int _tmain(int argc, _TCHAR **argv) {
#else
int main(int argc, char **argv) {
#endif

	/*
	 * Parse parameter for specific option
	 * if failed, exit.
	 * (sets player / program settings in here)
	 */
	if (!parse(argc, argv)) {
		LOG->Critical("Failed to parse parameter properly.");
		help();
		return -1;
	}

	/* 
	 * Game basic initalization
	 * (load option here)
	 * (our start scene is playing, cause it's simple bmx player..?)
	 */
	Game::Initialize();

	/*
	 * Start main scene
	 */
	SCENE->ChangeScene("Play");
	
	/* 
	 * Game main loop started
	 */
	Game::MainLoop();

	/*
	 * Okay, game end, release everything
	 */
	Game::Release();
	SDL_Quit();
	return 0;
}