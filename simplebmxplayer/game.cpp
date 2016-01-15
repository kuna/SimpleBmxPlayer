#include "game.h"
#include "gameplay.h"
#include "luamanager.h"
#include "util.h"
#include "globalresources.h"

// game status
GameSetting setting;

// bms info
std::wstring bmspath;

// SDL
SDL_Window* gWindow = NULL;
SDL_Renderer* renderer = NULL;

// drawing texture/fonts
SDL_Texture* note_texture;
FC_Font* font = NULL;

// FPS
Timer fpstimer;
int fps = 0;


void Game::Parameter::help() {
	wprintf(L"SimpleBmxPlayer\n================\n-- How to use -- \n\n"
		L"argument: (bmx file) (options ...)\n"
		L"options:"
		L"-noimage: don't load image files\n"
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

bool Game::Parameter::parse(int argc, _TCHAR **argv) {
	if (argc <= 1)
		return false;
	bmspath = argv[1];
	return true;
}

int test(Lua* l) {
	int a = (int)lua_tointeger(l, -1);
	lua_pop(l, 1);
	lua_pushinteger(l, a + 1);
	return 1;	// one return value
}

int test2(Lua* l) {
	RString str = (const char*)lua_tostring(l, -1);
	lua_pushinteger(l, str.size());
	return 1;	// one return value
}

std::map<RString, int> test_map;

bool Game::Init() {
	/*
	* Must init Timer first
	* - because various elements are uses timer using globalTick
	*/
	GameTimer::Tick();
	GameTimer::Start(0);

	/*
	 * Init instances
	 */
	PoolHelper::InitalizeAll();
	LUA = new LuaManager();

	// register func (test)
	//lua_pushcfunction(l, test);
	Lua *l;
	l = LUA->Get();
	lua_register(l, "test", test);
	lua_register(l, "test2", test2);
	LUA->Release(l);
	for (int i = 0; i < 1024; i++)
		test_map.insert(std::pair<RString, int>(ssprintf("Test%d", i), i));

	/*
	 * Load basic setting file ...
	 */
	if (!setting.LoadSetting("../setting.xml")) {
		wprintf(L"Cannot load settings files... Use Default settings...\n");
		if (!setting.DefaultSetting()) {
			wprintf(L"CANNOT load default settings. program exit.\n");
			return -1;
		}
	}
	
	/*
	 * Game engine initalize
	 */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		wprintf(L"Failed to SDL_Init() ...\n");
		return -1;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {	// no lag, no sound latency
		wprintf(L"Failed to Open Audio ...\n");
		return -1;
	}
	Mix_AllocateChannels(1296);
	gWindow = SDL_CreateWindow("SimpleBmxPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		setting.mEngine.mWidth, setting.mEngine.mHeight, SDL_WINDOW_SHOWN);
	if (!gWindow) {
		wprintf(L"Failed to create window\n");
		return -1;
	}
	renderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		wprintf(L"Failed to create Renderer\n");
		return -1;
	}

	/*
	 * prepare game basic resource
	 */
	font = FC_CreateFont();
	if (!font)
		return false;
	printf("Loading font ...\n");
	FC_LoadFont(font, renderer, "../skin/lazy.ttf", 28, FC_MakeColor(120, 120, 120, 255), TTF_STYLE_NORMAL);

	/*
	 * prepare GamePlay
	 */
	GamePlay::Init();
	// load BMS file
	if (GamePlay::LoadBms(bmspath)) {
		wprintf(L"Loaded BMS file successfully\n");
	}
	else {
		wprintf(L"cannot load BMS file\n");
		return false;
	}
	// start to load BMS resource file
	if (GamePlay::LoadBmsResource()) {
		wprintf(L"Loaded BMS resources successfully\n");
	}
	else {
		wprintf(L"cannot load BMS resource file\n");
		return false;
	}
	GamePlay::LoadSkin("../skin/Wisp_HD/play/HDPLAY_W.lr2skin");
	// prepare player
	PlayerSetting psetting;
	psetting.speed = 310;
	GamePlay::SetPlayer(psetting, 0);

	return true;
}

void Game::Start() {

	/*
	 * FPS timer start & initalize
	 */
	fpstimer.Start();
	fps = 0;
	
	/*
	 * GamePlay Start
	 * - We don't need select screen, only play screen.
	 */
	GamePlay::Start();
}

void Render_FPS() {
	// calculate FPS per 1 sec
	float avgfps = fps / (fpstimer.GetTick() / 1000.0f);
	fps++;
	// print FPS
	FC_Draw(font, renderer, 0, 0, "%.2f Frame", avgfps);
	if (fpstimer.GetTick() > 1000) {
		wprintf(L"%.2f\n", avgfps);
		fpstimer.Stop();
		fpstimer.Start();
		fps = 0;
	}
}

void Render() {
	// Lua test(benchmark)
	// execut func
	Lua *l = LUA->Get();
	for (int i = 0; i < 500; i++) {
#if 0
		RString e;
		//LuaHelpers::RunScript(l, "return 3", "test", e, 0, 1);
		//LuaHelpers::RunScript(l, "return test(3)", "test", e, 0, 1);
		LuaHelpers::RunScript(l, "return test2('abcde')", "test2", e, 0, 1);

		// get res
		int res = (int)lua_tointeger(l, -1);
		lua_pop(l, 1);
		//printf("%d\n", res);
#endif

		int k = test_map["Test0"];
		fps = fps + k;
	}
	LUA->Release(l);

	// nothing to do just render GamePlay ...
	GamePlay::Render();
}

void Game::MainLoop() {
	while (1) {
		/*
		 * Ticking
		 */
		GameTimer::Tick();

		/*
		 * Keybd, mouse event
		 */
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
			if (e.type == SDL_KEYUP) {
				// GamePlay::KeyPress(e.value);
			}
		}

		/*
		 * Graphic rendering
		 * - skin and movie(Image::Refresh) part are departed from main thread
		 *   so keypress will be out of lag
		 *   (TODO)
		 */
		SDL_RenderClear(renderer);
		Render();
		Render_FPS();
		SDL_RenderPresent(renderer);
	}
}

void Game::Release() {
	// other part release first
	GamePlay::Release();

	// game basic resource release
	// - FPS font
	FC_FreeFont(font);
	
	// release instances
	PoolHelper::ReleaseAll();
	delete LUA;

	// finally, audio/renderer close
	Mix_CloseAudio();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(gWindow);
}

SDL_Renderer* Game::GetRenderer() {
	return renderer;
}

SDL_Window* Game::GetWindow() {
	return gWindow;
}