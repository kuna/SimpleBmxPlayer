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
		SWITCH_ON("IsAutoPlay");
		SWITCH_ON("IsBGA");
		//SWITCH_ON("Is1PSuddenChange");
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
		 * must call at the end
		 */
		OnSongLoading->Start();
		OnScene->Start();
	}

	void RenderObject(SkinRenderObject *obj) {
		if (!obj->EvaluateCondition()) return;
		if (obj->IsGroup()) {
			// iterate all child
			SkinGroupObject *group = obj->ToGroup();
			group->SetAsRenderTarget();
			for (auto child = group->begin(); child != group->end(); ++child) {
				RenderObject(*child);
			}
			group->ResetRenderTarget();
			// if textured object, must call Render method.
			obj->Render();
		}
		else if (obj->IsGeneral()) {
			// let basic renderer do work
			obj->Render();
		}
		/* Ingeneral Objects: need special care! */
		else if (obj->ToBGA()) {
			obj->ToBGA()->RenderBGA(BmsResource::IMAGE.Get(1));
		}
		else if (obj->ToPlayObject()) {
			SkinPlayObject* play = obj->ToPlayObject();
			if (player[0]) player[0]->RenderNote(play);
			// if player[1] ~~
		}
		else {
			// ignore unknown object
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
		if (player[1]) player[1]->Update();

		/*
		 * draw interface (make a render tree recursion)
		 */
		RenderObject(rtree);

		/*
		 * draw basic skin elements
		 * - we'll going to comment it until Lua part is finished.
		 */
	#if 0
		for (auto it = playskin.begin(); it != playskin.end(); ++it) {
			(*it).GetRenderData(renderdata);
			if (!renderdata.img)
				continue;
			src.x = renderdata.src.x;
			src.y = renderdata.src.y;
			src.w = renderdata.src.w;
			src.h = renderdata.src.h;
			dest.x = renderdata.dst.x;
			dest.y = renderdata.dst.y;
			dest.w = renderdata.dst.w;
			dest.h = renderdata.dst.h;
			switch (renderdata.blend) {
			case 0:
				SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_NONE);
				break;
			case 1:
				SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_BLEND);
				break;
			case 2:
				SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_ADD);
				break;
			case 3:
				break;
			case 4:
				SDL_SetTextureBlendMode(renderdata.img->GetPtr(), SDL_BLENDMODE_MOD);
				break;
			}
			//SDL_SetTextureAlphaMod(renderdata.img->GetPtr(), 120);
			SDL_RenderCopy(Game::GetRenderer(), renderdata.img->GetPtr(), &src, &dest);
		}
	#endif

		/*
		 * we're currently working with skin parsing,
		 * so we'll going to block under these codes currently (related with playing)
		 */
	#if 0

		/*
		 * BGA rendering
		 */
		SDL_Rect bga_dst;
		playskin.bga.ToRect(bga_dst);
		if (bmsresource.IsBMPLoaded(bgavalue.ToInteger()))
			SDL_RenderCopy(Game::GetRenderer(), bmsresource.GetBMP(bgavalue.ToInteger())->GetPtr(), 0, &bga_dst);

		/*
		 * note rendering
		 */
		for (int pidx = 0; pidx < 2; pidx++) {
			// only for prepared player
			if (player[pidx].IsFinished())
				continue;
			// set player time(play sound) and ...
			player[pidx].SetTime(gametimer.GetTick());
			// get note pos
			double notepos = player[pidx].GetCurrentPos();
			// render notes
			int lnstartpos[20];
			bool lnstart[20];
			BmsTimeManager& bmstime = player[pidx].GetBmsTimeManager();
			BmsNoteContainer& bmsnote = player[pidx].GetBmsNotes();
			for (int i = 0; i < 20; i++) {
				// init lnstart
				dest.x = skin_note[i].dst.x;
				dest.w = skin_note[i].dst.w;
				src.x = skin_note[i].src.x;		src.y = skin_note[i].src.y;
				src.w = skin_note[i].src.w;		src.h = skin_note[i].src.h;
				src_lnbody.x = skin_lnbody[i].src.x;		src_lnbody.y = skin_lnbody[i].src.y;
				src_lnbody.w = skin_lnbody[i].src.w;		src_lnbody.h = 1;
				src_lnstart.x = skin_lnstart[i].src.x;		src_lnstart.y = skin_lnstart[i].src.y;
				src_lnstart.w = skin_lnstart[i].src.w;		src_lnstart.h = skin_lnstart[i].src.h;
				src_lnend.x = skin_lnend[i].src.x;		src_lnend.y = skin_lnend[i].src.y;
				src_lnend.w = skin_lnend[i].src.w;		src_lnend.h = skin_lnend[i].src.h;
				lnstartpos[i] = laneheight + lanestart;
				lnstart[i] = false;
				for (int nidx = player[pidx].GetCurrentNoteBar(i);
					nidx >= 0 && nidx < bmstime.GetSize();
					nidx = player[pidx].GetAvailableNoteIndex(i, nidx + 1)) {
					double ypos = laneheight - (bmstime[nidx].absbeat - notepos) * speed_multiply + lanestart;
					switch (bmsnote[i][nidx].type){
					case BmsNote::NOTE_NORMAL:
						dest.y = ypos;		dest.h = skin_note[i].dst.h;
						SDL_RenderCopy(Game::GetRenderer(), skin_note[i].img->GetPtr(), &src, &dest);
						break;
					case BmsNote::NOTE_LNSTART:
						lnstartpos[i] = ypos;
						lnstart[i] = true;
						break;
					case BmsNote::NOTE_LNEND:
						dest.y = ypos + skin_lnstart[i].dst.h;		dest.h = lnstartpos[i] - ypos - skin_lnstart[i].dst.h;
						SDL_RenderCopy(Game::GetRenderer(), skin_lnbody[i].img->GetPtr(), &src_lnbody, &dest);
						dest.y = lnstartpos[i];						dest.h = skin_lnstart[i].dst.h;
						SDL_RenderCopy(Game::GetRenderer(), skin_lnstart[i].img->GetPtr(), &src_lnstart, &dest);
						dest.y = ypos;								dest.h = skin_lnend[i].dst.h;
						SDL_RenderCopy(Game::GetRenderer(), skin_lnend[i].img->GetPtr(), &src_lnend, &dest);
						lnstart[i] = false;
						break;
					}
					// off the screen -> exit loop
					if (ypos < -lanestart)
						break;
				}
				// if LN_end hasn't found
				// then draw it to end of the screen
				if (lnstart[i]) {
					dest.y = lanestart + skin_lnstart[i].dst.h;		dest.h = lnstartpos[i] - 0 - skin_lnstart[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnbody[i].img->GetPtr(), &src_lnbody, &dest);
					dest.y = lnstartpos[i] + lanestart;				dest.h = skin_lnstart[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnstart[i].img->GetPtr(), &src_lnstart, &dest);
					dest.y = lanestart;								dest.h = skin_lnend[i].dst.h;
					SDL_RenderCopy(Game::GetRenderer(), skin_lnend[i].img->GetPtr(), &src_lnend, &dest);
				}
			}
		}
	#endif
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
