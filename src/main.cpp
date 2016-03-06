#include <tchar.h>
#include <wchar.h>
#include <string>

#include "util.h"
#include "file.h"
#include "Pool.h"
#include "game.h"
#include "gameplay.h"
#include "logger.h"
#include "Setting.h"


namespace Parameter {
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
		// (TODO)
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
		
		/*
		 * first figure out what player currently is
		 */
		Game::P.username = SETTING.username;//"NONAME";
		for (int i = 2; i < argc; i++) {
			if (BeginsWith(argv[i], "-n")) {
				Game::P.username = argv[i];
			}
		}

		/*
		 * Load player information
		 * This process should be made in SCENE::PLAYER originally.
		 */
		if (!PlayerInfoHelper::LoadPlayerInfo(PLAYERINFO[0], Game::P.username.c_str())) {
			LOG->Warn("Cannot find userdata %s. Set default.", Game::P.username.c_str());
			PLAYERINFO[0].name = SETTING.username;
			PlayerInfoHelper::DefaultPlayerInfo(PLAYERINFO[0]);
		}

		/*
		 * set default value from player / program settings
		 * TODO: depart bmspath / get bmshash here??
		 */
		std::vector<RString> courses;
		split(argv[1], ";", courses);
		for (int i = 0; i < courses.size(); i++) {
			/*
			 * before loading Bms file, check argument is folder
			 * if it does, get valid path from player
			 */
			RString hash = "";
			FILEMANAGER->SetFileFilter(".bml;.bme;.bms;.pms");
			// if archive, then mount it
			if (FileHelper::IsArchiveFileName(courses[i]))
				FILEMANAGER->Mount(courses[i]);
			if (FILEMANAGER->IsDirectory(courses[i])) {
				std::vector<RString> filelist;
				FILEMANAGER->GetDirectoryFileList(courses[i], filelist);
				{
					// select
					printf("Select one:\n");
					for (int i = 0; i < filelist.size(); i++) printf("%d. %s\n", i, filelist[i].c_str());
					char buf_[256];
					fflush(stdin);
					gets(buf_);
					int r = atoi(buf_);
					if (r >= filelist.size()) r = filelist.size() - 1;
					courses[i] = filelist[r];
				}
				FileBasic *f = FILEMANAGER->LoadFile(courses[i]);
				hash = f->GetMD5Hash();
				delete f;
			}
			FILEMANAGER->UnMount(courses[i]);

			hash = GetHash(courses[i]);

			GamePlay::P.bmspath[i] = courses[i];
			GamePlay::P.bmshash[i] = hash;
		}
		GamePlay::P.courseplay = courses.size();
		GamePlay::P.round = 1;
		//GamePlay::P.gauge = PLAYERINFO[0].playconfig.gaugetype;
		//GamePlay::P.op1 = PLAYERINFO[0].playconfig.op_1p;
		//GamePlay::P.op2 = PLAYERINFO[0].playconfig.op_2p;
		GamePlay::P.rate = 1;
		
		GamePlay::P.bga = SETTING.bga;
		GamePlay::P.startmeasure = 0;
		GamePlay::P.endmeasure = 1000;
		GamePlay::P.repeat = 1;
		GamePlay::P.replay = false;
		GamePlay::P.rseed = time(0) % 65536;
		GamePlay::P.pacemaker = 6.0 / 9.0;	// A rank
		GamePlay::P.isrecordable = true;

		// overwrite default options
		for (int i = 2; i < argc; i++) {
			if (BeginsWith(argv[i], "-op"))  {
				int op = atoi(argv[i] + 2);
				PLAYERINFO[0].playconfig.op_1p = op % 10;
				PLAYERINFO[0].playconfig.op_2p = op / 10;
			}
			else if (BeginsWith(argv[i], "-replay")) {
				GamePlay::P.replay = true;
				GamePlay::P.isrecordable = false;
			}
			else if (BeginsWith(argv[i], "-auto")) {
				GamePlay::P.autoplay = true;
				GamePlay::P.isrecordable = false;
			}
			else if (BeginsWith(argv[i], "-bgaoff")) {
				GamePlay::P.bga = false;
			}
			else if (BeginsWith(argv[i], "-rate")) {
				GamePlay::P.rate = atof(argv[i] + 5);
				GamePlay::P.isrecordable = false;
			}
			else if (BeginsWith(argv[i], "-pace")) {
				GamePlay::P.pacemaker = atof(argv[i] + 5);
			}
			else if (BeginsWith(argv[i], "-g")) {
				PLAYERINFO[0].playconfig.gaugetype = atoi(argv[i] + 2);
			}
			else if (BeginsWith(argv[i], "-s")) {
				GamePlay::P.startmeasure = atoi(argv[i] + 2);
				GamePlay::P.isrecordable = false;
			}
			else if (BeginsWith(argv[i], "-e")) {
				GamePlay::P.endmeasure = atoi(argv[i] + 2);
				GamePlay::P.isrecordable = false;
			}
			else if (BeginsWith(argv[i], "-r")) {
				GamePlay::P.repeat = atoi(argv[i] + 2);
				GamePlay::P.isrecordable = false;
			}
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
	if (!Parameter::parse(argc, argv)) {
		LOG->Critical("Failed to parse parameter properly.");
		Parameter::help();
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