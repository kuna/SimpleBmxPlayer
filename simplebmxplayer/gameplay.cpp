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

namespace GamePlay {
	// global resources
	Timer*				OnScene;
	Timer*				OnGameStart;
	Timer*				OnSongLoading;
	Timer*				OnSongLoadingEnd;
	Timer*				OnReady;
	RString*			Bmspath;
	RString*			PlaySkinpath;

	// bms skin resource
	// skin resource will be loaded in GlobalResource, 
	// so we don't need to take care of it
	Skin				playskin;
	SkinRenderTree*		rtree;
	SkinOption			skinoption;

	// bms play related
	Player*				player[2];	// player available up to 2

	double speed_multiply;

	SDL_Texture* temptexture;	// only for test purpose

	void Init() {
		// initalize
		rtree = new SkinRenderTree(1280, 760);

		// initalize global resources
		//HANDLERPOOL->Add("OnGamePlaySound", OnBmsSound);
		//HANDLERPOOL->Add("OnGamePlayBga", OnBmsBga);
		OnScene = TIMERPOOL->Set("OnScene", false);
		OnGameStart = TIMERPOOL->Set("OnGameStart", false);
		OnSongLoading = TIMERPOOL->Set("OnSongLoading", false);
		OnSongLoadingEnd = TIMERPOOL->Set("OnSongLoadingEnd", false);
		OnReady = TIMERPOOL->Set("OnReady", false);
		Bmspath = STRPOOL->Set("Bmspath");
		PlaySkinpath = STRPOOL->Set("PlaySkinpath");

		// make player & prepare
		player[0] = new PlayerAuto();
		player[1] = new Player();

		// temp resource
		int pitch;
		Uint32 *p;
		temptexture = SDL_CreateTexture(Game::RENDERER, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 20, 10);
		SDL_LockTexture(temptexture, 0, (void**)&p, &pitch);
		for (int i = 0; i < 20 * 10; i++)
			p[i] = (255 << 24 | 255 << 16 | 120 << 8 | 120);
		SDL_UnlockTexture(temptexture);
	}

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
		}
		if (!r)
		{
			printf("Failed to load skin %s\n", abspath);
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

		printf("Loaded Bms Skin Successfully\n");

		// prefetch note render information
		// COMMENT: setplayer should removed, move it to here
		// TODO

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

	void Start() {
		/*
		 * Load skin & bms resource
		 * (bms resource is loaded with thread)
		 * (Timers must created before this called)
		 */
		LoadSkin(*PlaySkinpath);
		LoadBms(*Bmspath);

		/*
		 * initalize timers
		 * MUST DO after skin is loaded
		 */
		GameTimer::Tick();
		SWITCH_ON("OnDiffAnother");
		SWITCH_ON("IsScoreGraph");
		SWITCH_OFF("IsAutoPlay");
		SWITCH_ON("IsBGA");
		//SWITCH_ON("Is1PSuddenChange");
		//SWITCH_ON("951");
		OnScene->Stop();
		OnSongLoadingEnd->Stop();
		OnReady->Stop();
		OnGameStart->Stop();
		OnSongLoading->Stop();

		/*
		 * BMS load end, so set player
		 */
		PlayerSetting psetting;
		psetting.speed = 310;
		player[0]->SetPlayerSetting(psetting);		// TODO: is this have any meaning?
		player[0]->SetSpeed(psetting.speed);
		player[0]->Prepare(0);

		/*
		 * must call at the end of the scene preparation
		 */
		OnSongLoading->Start();
		OnScene->Start();
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
			/* Ingeneral Objects: need special care! */
			else if (obj->ToBGA()) {
				obj->ToBGA()->RenderBGA(BmsHelper::GetMissBGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetLayer1BGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetLayer2BGA());
				obj->ToBGA()->RenderBGA(BmsHelper::GetMainBGA());
			}
			else if (obj->ToPlayObject()) {
				SkinPlayObject* play = obj->ToPlayObject();
				RenderGroup(play);								// draw other objects first
				//play->RenderLane(1, 0.2, 0.4);

				if (player[0]) player[0]->RenderNote(play);		// and draw note/judgeline/line ...
				// TODO: judgeline is also drawed in RenderGroup
				// TODD: method - SetJudgelineThickness()
				// if player[1] ~~
			}
			else {
				// ignore unknown object
			}
		}
	}

	void Render() {
		/*
		 * check timers
		 */
		if (OnReady->Trigger(OnSongLoadingEnd->IsStarted() && OnScene->GetTick() >= 3000))
			OnSongLoading->Stop();
		OnGameStart->Trigger(OnReady->GetTick() >= 2000);

		/*
		 * BMS update
		 */
		if (OnGameStart->IsStarted())
			BmsHelper::Update(OnGameStart->GetTick());

		/*
		 * Player update
		 */
		if (player[0]) player[0]->Update();
		//if (player[1]) player[1]->Update();

		/*
		 * draw interface (make a render tree recursion)
		 */
		RenderObject(rtree);
	}

	void Release() {
		// remove player
		if (player[0]) delete player[0];
		if (player[1]) delete player[1];

		// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
		// we don't need to clear BMS data until next BMS is loaded
		delete rtree;
		playskin.Release();
		//bmsresource.Clear();
		//bms.Clear();

		// temp resource
		if (temptexture) SDL_DestroyTexture(temptexture);
	}
}
