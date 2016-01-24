#include "gameplay.h"
#include "skin.h"
#include "skinrendertree.h"
#include "handlerargs.h"
#include "bmsbel/bms_bms.h"
#include "bmsbel/bms_parser.h"
#include "bmsresource.h"
#include "audio.h"
#include "image.h"
#include "game.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "player.h"
#include "game.h"

namespace GamePlay {
	// scene
	ScenePlay*			SCENE;
	
	// global resources
	Timer*				OnGameStart;
	Timer*				OnSongLoading;
	Timer*				OnSongLoadingEnd;
	Timer*				OnReady;
	Timer*				OnClose;			// when 1p & 2p dead
	Timer*				OnFadeIn;			// when game start
	Timer*				OnFadeOut;			// when game end
	Timer*				On1PMiss;			// just for missing image
	Timer*				On2PMiss;			// just for missing image

	RString*			Bmspath;

	// bms skin resource
	// skin resource will be loaded in GlobalResource, 
	// so we don't need to take care of it
	Skin				playskin;
	SkinRenderTree*		rtree;
	SkinOption			skinoption;

	// note
	BmsNoteContainer*	bmsnote;
	int					playmode;	// PLAYTYPE

	SDL_Texture* temptexture;	// only for test purpose

	bool LoadSkin(const char* path) {
		//SkinDST::On(33);		// autoplay on
		//SkinDST::On(41);		// BGA on

		// load play skin (lr2)
		// and create render tree
		RString abspath = path;
		FileHelper::ConvertPathToAbsolute(abspath);
		bool r = false;
		if (strstr(abspath, ".lr2skin") == 0) {
			r = playskin.Parse(path);
		}
		else {
			_LR2SkinParser *lr2skin = new _LR2SkinParser();
			r = lr2skin->ParseLR2Skin(abspath, &playskin);
			delete lr2skin;
			//playskin.Save("./test.xml");
		}
		if (!r)
		{
			LOG->Critical("Failed to load skin %s\n", abspath);
			return false;
		}

		// load skin (default) option, and set Environment.
		// MUST do before loading skin resource.
		playskin.GetDefaultOption(&skinoption);
		skinoption.SetEnvironmentFromOption();

		// load skin resource
		// - skin resources are relative to .xml file.
		RString skindirpath = IO::get_filedir(path);
		FileHelper::ConvertPathToAbsolute(skindirpath);
		FileHelper::PushBasePath(skindirpath);
		SkinRenderHelper::LoadResourceFromSkin(*rtree, playskin);
		SkinRenderHelper::ConstructTreeFromSkin(*rtree, playskin);
		FileHelper::PopBasePath();

		LOG->Info("Loaded Bms Skin Successfully\n");

		return true;
	}

	void LoadBms(const char *path) {
		if (BmsHelper::LoadBms(path)) {
			LOG->Info("BMS loading finished successfully (%s)\n", path);
			BmsHelper::LoadBmsResourceOnThread();
		}
		else {
			LOG->Critical("BMS loading failed (%s)\n", path);
		}
	}

	namespace {
		/* object rendering part */
		void RenderObject(SkinRenderObject*);
		void RenderGroup(SkinGroupObject *group) {
			// iterate a group
			group->SetAsRenderTarget();
			for (auto child = group->begin(); child != group->end(); ++child) {
				RenderObject(*child);
			}
			group->ResetRenderTarget();
			// if textured object, must call Render method.
			group->Render();
		}
		void RenderObject(SkinRenderObject *obj) {
			if (!obj->EvaluateCondition()) return;
			// update first and see ...
			obj->Update();
			if (obj->IsGroup()) {
				// iterate all child
				RenderGroup(obj->ToGroup());
			}
			else if (obj->IsGeneral()) {
				// let basic renderer do work
				obj->Render();
			}
			/*
			 * Under these are Ingeneral Objects: 
			 * need special care! 
			 */
			else if (obj->ToBGA()) {
				// TODO: check 1P / 2P
				if (On1PMiss->IsStarted())
					obj->ToBGA()->RenderBGA(BmsHelper::GetMissBGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetLayer1BGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetLayer2BGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetMainBGA());
			}
			else if (obj->ToPlayObject()) {
				SkinPlayObject* play = obj->ToPlayObject();
				RenderGroup(play);								// draw other objects first
				if (PLAYER[0]) PLAYER[0]->RenderNote(play);		// and draw note/judgeline/line ...
				if (PLAYER[1]) PLAYER[1]->RenderNote(play);
				// TODO: judgeline is also drawed in RenderGroup
				// TODD: method - SetJudgelineThickness()
			}
			else {
				// ignore unknown object
			}
		}
	}

	// sceneplay part
	void ScenePlay::Initialize() {
		// initalize skin tree
		rtree = new SkinRenderTree(1280, 760);

		// initalize global resources
		//HANDLERPOOL->Add("OnGamePlaySound", OnBmsSound);
		//HANDLERPOOL->Add("OnGamePlayBga", OnBmsBga);
		OnGameStart = SWITCH_OFF("OnGameStart");
		OnSongLoading = SWITCH_OFF("OnSongLoading");
		OnSongLoadingEnd = SWITCH_OFF("OnSongLoadingEnd");
		OnReady = SWITCH_OFF("OnReady");
		OnClose = SWITCH_OFF("OnClose");
		OnFadeIn = SWITCH_OFF("OnFadeIn");
		OnFadeOut = SWITCH_OFF("OnFadeOut");
		On1PMiss = SWITCH_OFF("On1PMiss");
		On2PMiss = SWITCH_OFF("On2PMiss");
		Bmspath = STRPOOL->Get("Bmspath");

		// temp resource
		int pitch;
		Uint32 *p;
		temptexture = SDL_CreateTexture(Game::RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 20, 10);
		SDL_LockTexture(temptexture, 0, (void**)&p, &pitch);
		for (int i = 0; i < 20 * 10; i++)
			p[i] = (255 << 24 | 255 << 16 | 120 << 8 | 120);
		SDL_UnlockTexture(temptexture);
	}

	void ScenePlay::Start() {
		/*
		 * Load bms resource first
		 * (bms resource is loaded with thread)
		 * (load bms first to find out what key skin is proper)
		 */
		LoadBms(*Bmspath);
		RString PlayskinPath = "";
		playmode = BmsResource::BMS.GetKey();
		bmsnote = new BmsNoteContainer();
		BmsResource::BMS.GetNotes(*bmsnote);

		/*
		 * Load skin
		 */
		if (playmode < 10)
			PlayskinPath = Game::SETTING.skin_play_7key;
		else
			PlayskinPath = Game::SETTING.skin_play_14key;
		LoadSkin(PlayskinPath);

		/*
		 * Create player object for playing
		 */
		PLAYER[0] = new PlayerAuto(&PLAYERINFO[0].playconfig, bmsnote, 0, playmode);
		PLAYER[1] = NULL;

		/*
		 * initalize timers
		 * MUST DO after skin is loaded
		 */
		GameTimer::Tick();
		SWITCH_ON("OnDiffAnother");
		SWITCH_ON("IsScoreGraph");
		SWITCH_OFF("IsAutoPlay");
		SWITCH_ON("IsBGA");
		SWITCH_ON("IsExtraMode");
		SWITCH_ON("OnDiffInsane");
		//SWITCH_ON("Is1PSuddenChange");
		//SWITCH_ON("981");
		DOUBLEPOOL->Set("TargetExScore", 0.5);
		DOUBLEPOOL->Set("TargetExScore", 0.5);
		OnSongLoadingEnd->Stop();
		OnReady->Stop();
		OnGameStart->Stop();
		OnSongLoading->Stop();

		/*
		 * must call at the end of the scene preparation
		 */
		OnSongLoading->Start();

	}

	void ScenePlay::Update() {
		/*
		 * check timers (game flow related)
		 */
		if (OnReady->Trigger(OnSongLoadingEnd->IsStarted() && OnSongLoading->GetTick() >= 3000))
			OnSongLoading->Stop();
		OnGameStart->Trigger(OnReady->GetTick() >= 2000);
		// OnClose is called when all player is dead
		bool close = true;
		if (close && PLAYER[0]) close = close && PLAYER[0]->IsDead();
		if (close && PLAYER[1]) close = close && PLAYER[1]->IsDead();
		OnClose->Trigger(close);
		// OnFadeout is called when endtime is over
		OnFadeOut->Trigger(BmsHelper::GetEndTime() + 2000 < OnGameStart->GetTick());
		// If OnClose/OnFadeout has enough time, then go to next scene (End here)
		if (OnClose->GetTick() > 3000 || OnFadeOut->GetTick() > 3000)
			Game::End();

		/*
		 * BMS update
		 */
		if (OnGameStart->IsStarted())
			BmsHelper::Update(OnGameStart->GetTick());

		/*
		 * Player update
		 */
		if (PLAYER[0]) PLAYER[0]->Update();
		if (PLAYER[1]) PLAYER[1]->Update();
	}

	void ScenePlay::Render() {
		/*
		* draw interface (make a render tree recursion)
		*/
		RenderObject(rtree);
	}

	void ScenePlay::End() {
		/*
		 * just save player properties when game goes end
		 */
		PlayerInfoHelper::SavePlayerInfo(PLAYERINFO[0]);
	}

	void ScenePlay::Release() {
		// remove player & note
		SAFE_DELETE(bmsnote);
		SAFE_DELETE(PLAYER[0]);
		SAFE_DELETE(PLAYER[1]);

		// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
		// we don't need to clear BMS data until next BMS is loaded
		delete rtree;
		playskin.Release();
		//bmsresource.Clear();
		//bms.Clear();

		// temp resource
		if (temptexture) SDL_DestroyTexture(temptexture);
	}

	/** private */
	int GetChannelFromFunction(int func) {
		int channel = 0;
		switch (func) {
		case PlayerKeyIndex::P1_BUTTON1:
			channel = 1;
			break;
		case PlayerKeyIndex::P1_BUTTON2:
			channel = 2;
			break;
		case PlayerKeyIndex::P1_BUTTON3:
			channel = 3;
			break;
		case PlayerKeyIndex::P1_BUTTON4:
			channel = 4;
			break;
		case PlayerKeyIndex::P1_BUTTON5:
			channel = 5;
			break;
		case PlayerKeyIndex::P1_BUTTON6:
			channel = 8;
			break;
		case PlayerKeyIndex::P1_BUTTON7:
			channel = 9;
			break;
		case PlayerKeyIndex::P1_BUTTON8:
			channel = 6;
			break;
		case PlayerKeyIndex::P1_BUTTON9:
			channel = 7;
			break;
		case PlayerKeyIndex::P2_BUTTON1:
			channel = 11;
			break;
		case PlayerKeyIndex::P2_BUTTON2:
			channel = 12;
			break;
		case PlayerKeyIndex::P2_BUTTON3:
			channel = 13;
			break;
		case PlayerKeyIndex::P2_BUTTON4:
			channel = 14;
			break;
		case PlayerKeyIndex::P2_BUTTON5:
			channel = 15;
			break;
		case PlayerKeyIndex::P2_BUTTON6:
			channel = 18;
			break;
		case PlayerKeyIndex::P2_BUTTON7:
			channel = 19;
			break;
		case PlayerKeyIndex::P2_BUTTON8:
			channel = 16;
			break;
		case PlayerKeyIndex::P2_BUTTON9:
			channel = 17;
			break;
		}
		return channel;
	}

	// lane is changed instead of sudden if you press CTRL key
	bool pressedCtrl = false;
	void ScenePlay::KeyDown(int code, bool repeating) {
		/*
		 * -- some presets --
		 *
		 * F11/F12 (start+VEFX) press
		 * - if float -> OFF, reset speed (check SETTING::SpeedDelta)
		 * - if float -> ON, reset speed(refresh floatspeed) & just turn on float value.
		 *
		 * -- basic actions --
		 *
		 * start key press
		 * - if float == on, reset speed from floating
		 * speed change (arrow key / WKEY+START)
		 * - SETTING::Speeddelta
		 */
		int func = PlayerKeyIndex::NONE;
		switch (code) {
		case SDLK_F12:
			// refresh delta speed and toggle floating mode
			PLAYER[0]->DeltaSpeed(0);
			PLAYERINFO[0].playconfig.usefloatspeed
				= !PLAYERINFO[0].playconfig.usefloatspeed;
			break;
		case SDLK_UP:
			// ONLY 1P. may need to PRS+WKEY if you need to control 2P.
			if (PLAYERINFO[0].playconfig.usefloatspeed)
				PLAYER[0]->DeltaFloatSpeed(0.01);
			else
				PLAYER[0]->DeltaSpeed(0.05);
			break;
		case SDLK_DOWN:
			if (PLAYERINFO[0].playconfig.usefloatspeed)
				PLAYER[0]->DeltaFloatSpeed(-0.01);
			else
				PLAYER[0]->DeltaSpeed(-0.05);
			break;
		case SDLK_RIGHT:
			if (pressedCtrl) PLAYER[0]->DeltaSudden(0.05);
			else PLAYER[0]->DeltaLift(0.05);
			break;
		case SDLK_LEFT:
			if (pressedCtrl) PLAYER[0]->DeltaSudden(-0.05);
			else PLAYER[0]->DeltaLift(-0.05);
			break;
		case SDLK_LCTRL:
			pressedCtrl = true;
			break;
		default:
			func = PlayerKeyHelper::GetKeyCodeFunction(PLAYERINFO[0].keyconfig, code);
			// change it to key channel ...
			int channel = GetChannelFromFunction(func);
			if (PLAYER[0]) PLAYER[0]->PressKey(channel);
			if (PLAYER[1]) PLAYER[1]->PressKey(channel);
		}
	}

	void ScenePlay::KeyUp(int code) {
		int func = PlayerKeyIndex::NONE;
		switch (code) {
		case SDLK_LCTRL:
			pressedCtrl = false;
			break;
		default:
			func = PlayerKeyHelper::GetKeyCodeFunction(PLAYERINFO[0].keyconfig, code);
			int channel = GetChannelFromFunction(func);
			if (PLAYER[0]) PLAYER[0]->UpKey(channel);
			if (PLAYER[1]) PLAYER[1]->UpKey(channel);
		}
	}
}
