#include "gameplay.h"
#include "skin.h"
#include "ActorRenderer.h"
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
#include "playerinfo.h"
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

	int					playmode;			// PLAYTYPE..?

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
		rtree->SetObject(playskin.skinlayout.FirstChildElement("Info"));
		SkinRenderHelper::LoadResourceFromSkin(*rtree, playskin);
		SkinRenderHelper::ConstructTreeFromSkin(*rtree, playskin);
		FileHelper::PopBasePath();

		LOG->Info("Loaded Bms Skin Successfully\n");

		return true;
	}

	void LoadBms(const char *path) {
		if (BmsHelper::LoadBms(path)) {
			LOG->Info("BMS loading finished successfully (%s)\n", path);
		}
		else {
			LOG->Critical("BMS loading failed (%s)\n", path);
		}
	}

	namespace {
		void RenderObject(SkinRenderObject *obj) {
			obj->Update();
			obj->Render();
		}
	}

	namespace {
		void Initalize_BmsValue() {
			BMSVALUE.songloadprogress = DOUBLEPOOL->Get("SongLoadProgress");
			BMSVALUE.OnSongLoading = TIMERPOOL->Get("OnSongLoading");
			BMSVALUE.OnSongLoadingEnd = TIMERPOOL->Get("OnSongLoadingEnd");

			BMSVALUE.PlayProgress = DOUBLEPOOL->Get("PlayProgress");
			BMSVALUE.PlayBPM = INTPOOL->Get("PlayBPM");
			BMSVALUE.PlayMin = INTPOOL->Get("PlayMinute");
			BMSVALUE.PlaySec = INTPOOL->Get("PlaySecond");
			BMSVALUE.PlayRemainSec = INTPOOL->Get("PlayRemainSecond");
			BMSVALUE.PlayRemainMin = INTPOOL->Get("PlayRemainMinute");

			BMSVALUE.OnBeat = TIMERPOOL->Get("OnBeat");
			BMSVALUE.OnBgaMain = TIMERPOOL->Get("OnBgaMain");
			BMSVALUE.OnBgaLayer1 = TIMERPOOL->Get("OnBgaLayer1");
			BMSVALUE.OnBgaLayer2 = TIMERPOOL->Get("OnBgaLayer2");
		}

		void Initalize_P1_RenderValue() {
			PLAYERVALUE[0].pNoteSpeed = INTPOOL->Get("P1Speed");
			PLAYERVALUE[0].pFloatSpeed = INTPOOL->Get("P1FloatSpeed");
			PLAYERVALUE[0].pSudden = INTPOOL->Get("P1Sudden");
			PLAYERVALUE[0].pLift = INTPOOL->Get("P1Lift");
			PLAYERVALUE[0].pSudden_d = DOUBLEPOOL->Get("P1Sudden");
			PLAYERVALUE[0].pLift_d = DOUBLEPOOL->Get("P1Lift");

			PLAYERVALUE[0].pGauge_d = DOUBLEPOOL->Get("P1Gauge");
			PLAYERVALUE[0].pGaugeType = INTPOOL->Get("P1GaugeType");
			PLAYERVALUE[0].pGauge = INTPOOL->Get("P1Gauge");
			PLAYERVALUE[0].pExscore = INTPOOL->Get("P1ExScore");
			PLAYERVALUE[0].pScore = INTPOOL->Get("P1Score");
			PLAYERVALUE[0].pExscore_d = DOUBLEPOOL->Get("P1ExScore");
			PLAYERVALUE[0].pHighscore_d = DOUBLEPOOL->Get("P1HighScore");
			PLAYERVALUE[0].pScore = INTPOOL->Get("P1Score");
			PLAYERVALUE[0].pCombo = INTPOOL->Get("P1Combo");
			PLAYERVALUE[0].pMaxCombo = INTPOOL->Get("P1MaxCombo");
			PLAYERVALUE[0].pTotalnotes = INTPOOL->Get("P1TotalNotes");
			PLAYERVALUE[0].pRivaldiff = INTPOOL->Get("P1RivalDiff");
			PLAYERVALUE[0].pRate = INTPOOL->Get("P1Rate");
			PLAYERVALUE[0].pTotalRate = INTPOOL->Get("P1TotalRate");
			PLAYERVALUE[0].pRate_d = DOUBLEPOOL->Get("P1Rate");
			PLAYERVALUE[0].pTotalRate_d = DOUBLEPOOL->Get("P1TotalRate");

			PLAYERVALUE[0].pOnJudge[5] = TIMERPOOL->Set("OnP1JudgePerfect");
			PLAYERVALUE[0].pOnJudge[4] = TIMERPOOL->Set("OnP1JudgeGreat");
			PLAYERVALUE[0].pOnJudge[3] = TIMERPOOL->Set("OnP1JudgeGood");
			PLAYERVALUE[0].pOnJudge[2] = TIMERPOOL->Set("OnP1JudgeBad");
			PLAYERVALUE[0].pOnJudge[1] = TIMERPOOL->Set("OnP1JudgePoor");
			PLAYERVALUE[0].pOnJudge[0] = TIMERPOOL->Set("OnP1JudgePoor");

			PLAYERVALUE[0].pOnMiss = TIMERPOOL->Get("OnP1Miss");
			PLAYERVALUE[0].pOnCombo = TIMERPOOL->Get("OnP1Combo");
			PLAYERVALUE[0].pOnfullcombo = TIMERPOOL->Get("OnP1FullCombo");
			PLAYERVALUE[0].pOnlastnote = TIMERPOOL->Get("OnP1LastNote");
			PLAYERVALUE[0].pOnGameover = TIMERPOOL->Get("OnP1GameOver");
			PLAYERVALUE[0].pOnGaugeMax = TIMERPOOL->Get("OnP1GaugeMax");
			PLAYERVALUE[0].pOnGaugeUp = TIMERPOOL->Get("OnP1GaugeUp");

			/*
			 * SC : note-index 0
			 */
			PLAYERVALUE[0].pLanepress[0] = TIMERPOOL->Get("OnP1KeySCPress");
			PLAYERVALUE[0].pLaneup[0] = TIMERPOOL->Get("OnP1KeySCUp");
			PLAYERVALUE[0].pLanehold[0] = TIMERPOOL->Get("OnP1JudgeSCHold");
			PLAYERVALUE[0].pLanejudgeokay[0] = TIMERPOOL->Get("OnP1JudgeSCOkay");
			for (int i = 1; i < 10; i++) {
				PLAYERVALUE[0].pLanepress[i] = TIMERPOOL->Get(ssprintf("OnP1Key%dPress", i));
				PLAYERVALUE[0].pLaneup[i] = TIMERPOOL->Get(ssprintf("OnP1Key%dUp", i));
				PLAYERVALUE[0].pLanehold[i] = TIMERPOOL->Get(ssprintf("OnP1Judge%dHold", i));
				PLAYERVALUE[0].pLanejudgeokay[i] = TIMERPOOL->Get(ssprintf("OnP1Judge%dOkay", i));
			}
		}

		void Initalize_P2_RenderValue() {
			PLAYERVALUE[1].pNoteSpeed = INTPOOL->Get("P2Speed");
			PLAYERVALUE[1].pFloatSpeed = INTPOOL->Get("P2FloatSpeed");
			PLAYERVALUE[1].pSudden = INTPOOL->Get("P2Sudden");
			PLAYERVALUE[1].pSudden_d = DOUBLEPOOL->Get("P2Sudden");
			PLAYERVALUE[1].pLift = INTPOOL->Get("P2Lift");
			PLAYERVALUE[1].pLift_d = DOUBLEPOOL->Get("P2Lift");

			PLAYERVALUE[1].pGauge_d = DOUBLEPOOL->Get("P2Gauge");
			PLAYERVALUE[1].pGaugeType = INTPOOL->Get("P2GaugeType");
			PLAYERVALUE[1].pGauge = INTPOOL->Get("P2Gauge");
			PLAYERVALUE[1].pExscore = INTPOOL->Get("P2ExScore");
			PLAYERVALUE[1].pScore = INTPOOL->Get("P2Score");
			PLAYERVALUE[1].pExscore_d = DOUBLEPOOL->Get("P2ExScore");
			PLAYERVALUE[1].pHighscore_d = DOUBLEPOOL->Get("P2HighScore");
			PLAYERVALUE[1].pScore = INTPOOL->Get("P2Score");
			PLAYERVALUE[1].pCombo = INTPOOL->Get("P2Combo");
			PLAYERVALUE[1].pMaxCombo = INTPOOL->Get("P2MaxCombo");
			PLAYERVALUE[1].pTotalnotes = INTPOOL->Get("P2TotalNotes");
			PLAYERVALUE[1].pRivaldiff = INTPOOL->Get("P2RivalDiff");
			PLAYERVALUE[1].pRate = INTPOOL->Get("P2Rate");
			PLAYERVALUE[1].pTotalRate = INTPOOL->Get("P2TotalRate");
			PLAYERVALUE[1].pRate_d = DOUBLEPOOL->Get("P2Rate");
			PLAYERVALUE[1].pTotalRate_d = DOUBLEPOOL->Get("P2TotalRate");

			PLAYERVALUE[1].pOnJudge[5] = TIMERPOOL->Set("OnP2JudgePerfect");
			PLAYERVALUE[1].pOnJudge[4] = TIMERPOOL->Set("OnP2JudgeGreat");
			PLAYERVALUE[1].pOnJudge[3] = TIMERPOOL->Set("OnP2JudgeGood");
			PLAYERVALUE[1].pOnJudge[2] = TIMERPOOL->Set("OnP2JudgeBad");
			PLAYERVALUE[1].pOnJudge[1] = TIMERPOOL->Set("OnP2JudgePoor");
			PLAYERVALUE[1].pOnJudge[0] = TIMERPOOL->Set("OnP2JudgePoor");

			PLAYERVALUE[1].pOnMiss = TIMERPOOL->Get("OnP2Miss");
			PLAYERVALUE[1].pOnCombo = TIMERPOOL->Get("OnP2Combo");
			PLAYERVALUE[1].pOnfullcombo = TIMERPOOL->Get("OnP2FullCombo");
			PLAYERVALUE[1].pOnlastnote = TIMERPOOL->Get("OnP2LastNote");
			PLAYERVALUE[1].pOnGameover = TIMERPOOL->Get("OnP2GameOver");
			PLAYERVALUE[1].pOnGaugeMax = TIMERPOOL->Get("OnP2GaugeMax");
			PLAYERVALUE[1].pOnGaugeUp = TIMERPOOL->Get("OnP2GaugeUp");

			PLAYERVALUE[1].pLanepress[0] = TIMERPOOL->Get("OnP2KeySCPress");
			PLAYERVALUE[1].pLaneup[0] = TIMERPOOL->Get("OnP2KeySCUp");
			PLAYERVALUE[1].pLanehold[0] = TIMERPOOL->Get("OnP2JudgeSCHold");
			PLAYERVALUE[1].pLanejudgeokay[0] = TIMERPOOL->Get("OnP2JudgeSCOkay");
			for (int i = 1; i < 10; i++) {
				PLAYERVALUE[1].pLanepress[i] = TIMERPOOL->Get(ssprintf("OnP2Key%dPress", i));
				PLAYERVALUE[1].pLaneup[i] = TIMERPOOL->Get(ssprintf("OnP2Key%dUp", i));
				PLAYERVALUE[1].pLanehold[i] = TIMERPOOL->Get(ssprintf("OnP2Judge%dHold", i));
				PLAYERVALUE[1].pLanejudgeokay[i] = TIMERPOOL->Get(ssprintf("OnP2Judge%dOkay", i));
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

		// initalize player rendering values
		Initalize_BmsValue();
		Initalize_P1_RenderValue();
		Initalize_P2_RenderValue();
	}

	void ScenePlay::Start() {
		/*
		 * initalize timers
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
		 * Load bms first
		 * (bms resource is loaded with thread)
		 * (load bms first to find out what key skin is proper)
		 */
		LoadBms(*Bmspath);
		playmode = BmsResource::BMS.GetKey();

		/*
		 * apply some switches/values about BMS
		 * - if BACKBMP, then SET and load _backbmp
		 */
		std::string title = "";
		std::string subtitle = "";
		std::string genre = "";
		std::string artist = "";
		std::string subartist = "";
		BmsResource::BMS.GetHeaders().Query("TITLE", title);
		BmsResource::BMS.GetHeaders().Query("SUBTITLE", subtitle);
		BmsResource::BMS.GetHeaders().Query("ARTIST", artist);
		BmsResource::BMS.GetHeaders().Query("SUBARTIST", subartist);
		BmsResource::BMS.GetHeaders().Query("GENRE", genre);
		std::string maintitle = title;
		if (subtitle.size())
			maintitle = maintitle + " [" + subtitle + "]";

		STRPOOL->Set("MainTitle", maintitle);
		STRPOOL->Set("Title", title);
		STRPOOL->Set("Subtitle", subtitle);
		STRPOOL->Set("Genre", genre);
		STRPOOL->Set("Artist", artist);
		STRPOOL->Set("SubArtist", subartist);
		SWITCH_OFF("IsBACKBMP");

		/*
		 * Create player object for playing
		 * MUST create before load skin
		 */
		PLAYER[0] = new Player(0, playmode);
		PLAYER[1] = NULL;

		/*
		 * Load skin
		 */
		RString PlayskinPath = "";
		if (playmode < 10)
			PlayskinPath = Game::SETTING.skin_play_7key;
		else
			PlayskinPath = Game::SETTING.skin_play_14key;
		LoadSkin(PlayskinPath);

		/*
		 * Load bms resource
		 */
#if _DEBUG
		BmsHelper::LoadBmsResource();
#else
		BmsHelper::LoadBmsResourceOnThread();
#endif

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
		OnFadeOut->Trigger(BmsHelper::GetEndTime() < OnGameStart->GetTick());
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
		// remove player
		SAFE_DELETE(PLAYER[0]);
		SAFE_DELETE(PLAYER[1]);

		// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
		// we don't need to clear BMS data until next BMS is loaded
		delete rtree;
		playskin.Release();
		//bmsresource.Clear();
		//bms.Clear();
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
			channel = 6;
			break;
		case PlayerKeyIndex::P1_BUTTON7:
			channel = 7;
			break;
		case PlayerKeyIndex::P1_BUTTON8:
			channel = 0;
			break;
		case PlayerKeyIndex::P1_BUTTON9:
			channel = 9;
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
			channel = 16;
			break;
		case PlayerKeyIndex::P2_BUTTON7:
			channel = 17;
			break;
		case PlayerKeyIndex::P2_BUTTON8:
			channel = 10;
			break;
		case PlayerKeyIndex::P2_BUTTON9:
			channel = 19;
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
			if (pressedCtrl) PLAYER[0]->DeltaLift(0.05);
			else PLAYER[0]->DeltaSudden(0.05);
			break;
		case SDLK_LEFT:
			if (pressedCtrl) PLAYER[0]->DeltaLift(-0.05);
			else PLAYER[0]->DeltaSudden(-0.05);
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
