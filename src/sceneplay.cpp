#include "Sceneplay.h"
#include "skin.h"
#include "bmsbel/bms_bms.h"
#include "bmsbel/bms_parser.h"
#include "SongPlayer.h"
#include "Song.h"
#include "audio.h"
#include "game.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "player.h"
#include "playerinfo.h"
#include "Setting.h"
#include "game.h"
#include "Sceneresult.h"

// scene
ScenePlay*			SCENEPLAY;

namespace {
	int *pRivalDiff;
	double *pRivalDiff_d;
	double *pRivalDiff_d_total;

	void InitalizePlayer() {
		/*
			* delete player if previously existed
			*/
		if (PLAYER[0]) SAFE_DELETE(PLAYER[0]);	// player
		if (PLAYER[1]) SAFE_DELETE(PLAYER[1]);	// 2p(battlemode) or auto-player(pacemaker)
		if (PLAYER[2]) SAFE_DELETE(PLAYER[2]);	// replay(mybest)

		/*
			* Create player object for playing
			* MUST create before load skin
			* MUST create after Bms loaded (rate configured)
			*/
		// gauge rseed op1 op2 setting (in player)
		// in courseplay, player object shouldn't be created many times
		// so, before scene start, check round, and decide to create player.
		if (m_Autoplay) {
			PLAYER[0] = new PlayerAuto(0);
		}
		else if (P.replay) {
			/*
			* in case of course mode,
			* replay will be stored in course folder
			* (TODO)
			*/
			PlayerReplayRecord rep;
			if (!PlayerReplayHelper::LoadReplay(rep, PLAYERINFO[0].name, P.bmshash[0])) {
				LOG->Critical("Failed to load replay file.");
				return;		// ??
			}
			PlayerReplay *pRep = new PlayerReplay(0);
			PLAYER[0] = pRep;
			pRep->SetReplay(rep);
		}
		else {
			PLAYER[0] = new Player(0);
		}
		// other side is pacemaker
		PLAYER[1] = new PlayerAuto(1);	// MUST always single?
		PLAYER[1]->SetPlaySound(false);	// Pacemaker -> Silent!
		((PlayerAuto*)PLAYER[1])->SetGoal(P.pacemaker);

		// generate replay object
		PLAYER[2] = new PlayerReplay(2);
	}
}

void ScenePlay::Start() {
	/* ---------------------------------------------------------------------------
		* Load settings from parameter
		* Load bms first
		* (bms resource is loaded with thread)
		* (load bms first to find out what key skin is proper)
		* COMMENT: if bms load failed, then continue with empty bms file.
		*          Make SongInfo structure, and load detailed information to there.
		*/
	BmsHelper::SetLoadOption(
		P.startmeasure,
		P.endmeasure,
		P.repeat,
		P.bga
	);

	BmsBms bms;
	RString bmspath = P.bmspath[P.round - 1];
	if (BmsHelper::LoadBms(bmspath, bms)) {
		LOG->Info("BMS loading finished successfully (%s)\n", bmspath.c_str());
	}
	else {
		LOG->Critical("BMS loading failed (%s)\n", bmspath.c_str());
	}
	SONGPLAYER->SetRate(P.rate);



	/* ---------------------------------------------------------------------------
		* Set Players after Bms is loaded
		*/

	/* if round 1, then create(reset) player object */
	if (P.round == 1)
		InitalizePlayer();

	//
	// bms note should be created, 
	// when creating note data, cautions:
	// - MUST Bms file is loaded
	// - ONLY rseed is argument. op is setted in player information.
	// - each player has its own note data.
	//
	if (PLAYER[0]) PLAYER[0]->InitalizeNote(&bms);
	if (PLAYER[1]) PLAYER[1]->InitalizeNote(&bms);	// COMMENT: player 2 may use different bms file if is battle mode ...
	if (PLAYER[2]) PLAYER[2]->InitalizeNote(&bms);


	// load player record
	if (PlayerRecordHelper::LoadPlayerRecord(
		record, PLAYERINFO[0].name, P.bmshash[P.round - 1]
		)) {
		//
		// TODO
		// set switch (hd cleared? mybest? etc ...)
		//
		INTPOOL->Set("SongClear", record.status);

		//
		// create mybest object
		//
		PLAYER[1] = new PlayerAuto(1);	// MUST always single?
		PLAYER[1]->SetPlaySound(false);
		((PlayerAuto*)PLAYER[1])->SetGoal(record.score.CalculateRate());

		//
		// in courseplay, gauge shouldn't be cleared
		// after sequential round
		//
		if (PLAYER[0]) PLAYER[0]->InitalizeGauge();
		if (PLAYER[1]) PLAYER[1]->InitalizeGauge();
	}



	/* ---------------------------------------------------------------------------
	* Set rendervalue/switches before skin is loaded
	*/
	GameTimer::Tick();
	PLAYVALUE.OnSongLoadingEnd->Stop();
	PLAYVALUE.OnReady->Stop();
	PLAYVALUE.OnSongStart->Stop();
	PLAYVALUE.OnSongLoading->Stop();

	SongInfo songinfo;
	BmsHelper::GetBmsMetadata(bms, songinfo);

	SONGVALUE.sMainTitle->assign(songinfo.sMainTitle);
	SONGVALUE.sTitle->assign(songinfo.sTitle);
	SONGVALUE.sSubTitle->assign(songinfo.sSubTitle);
	SONGVALUE.sGenre->assign(songinfo.sGenre);
	SONGVALUE.sArtist->assign(songinfo.sArtist);
	SONGVALUE.sSubArtist->assign(songinfo.sSubArtist);

	SWITCH_OFF("OnDiffBeginner");
	SWITCH_OFF("OnDiffNormal");
	SWITCH_OFF("OnDiffHyper");
	SWITCH_OFF("OnDiffAnother");
	SWITCH_OFF("OnDiffInsane");
	switch (songinfo.iDifficulty) {
	case 0:
		SWITCH_ON("OnDiffNone");
		break;
	case 1:
		SWITCH_ON("OnDiffBeginner");
		break;
	case 2:
		SWITCH_ON("OnDiffNormal");
		break;
	case 3:
		SWITCH_ON("OnDiffHyper");
		break;
	case 4:
		SWITCH_ON("OnDiffAnother");
		break;
	case 5:
	default:
		SWITCH_ON("OnDiffInsane");
		break;
	}
	INTPOOL->Set("PlayLevel", songinfo.iLevel);

	if (songinfo.sBackBmp.size()) {
		// TODO: load backbmp
		SWITCH_ON("IsBACKBMP");
	}
	else {
		SWITCH_OFF("IsBACKBMP");
	}



	SWITCH_OFF("IsGhostOff");
	SWITCH_OFF("IsGhostA");
	SWITCH_OFF("IsGhostB");
	SWITCH_OFF("IsGhostC");
	switch (PLAYERINFO[0].playconfig.ghost_type) {
	case GHOSTTYPE::OFF:
		SWITCH_ON("IsGhostOff");
		break;
	case GHOSTTYPE::TYPEA:
		SWITCH_ON("IsGhostA");
		break;
	case GHOSTTYPE::TYPEB:
		SWITCH_ON("IsGhostB");
		break;
	case GHOSTTYPE::TYPEC:
		SWITCH_ON("IsGhostC");
		break;
	}
	SWITCH_OFF("IsJudgeOff");
	SWITCH_OFF("IsJudgeA");
	SWITCH_OFF("IsJudgeB");
	SWITCH_OFF("IsJudgeC");
	switch (PLAYERINFO[0].playconfig.judge_type) {
	case JUDGETYPE::OFF:
		SWITCH_ON("IsJudgeOff");
		break;
	case JUDGETYPE::TYPEA:
		SWITCH_ON("IsJudgeA");
		break;
	case JUDGETYPE::TYPEB:
		SWITCH_ON("IsJudgeB");
		break;
	case JUDGETYPE::TYPEC:
		SWITCH_ON("IsJudgeC");
		break;
	}
	switch (PLAYERINFO[0].playconfig.pacemaker_type) {
	case PACEMAKERTYPE::PACE0:
		break;
	}
	SWITCH_ON("IsScoreGraph");
	SWITCH_OFF("IsAutoPlay");
	SWITCH_ON("IsBGA");
	SWITCH_ON("IsExtraMode");
	DOUBLEPOOL->Set("TargetExScore", 0.5);
	DOUBLEPOOL->Set("TargetExScore", 0.5);
	INTPOOL->Set("MyBest", 12);		// TODO



	/* ---------------------------------------------------------------------------
		* Load skin
		* - from here, something will show up in the screen.
		*/
	RString PlayskinPath = "";
	if (playmode < 10)
		PlayskinPath = SETTING.skin_play_7key;
	else
		PlayskinPath = SETTING.skin_play_14key;
	if (!theme.Load(PlayskinPath))
		LOG->Critical("Failed to load play skin: %s", PlayskinPath.c_str());



	/* ---------------------------------------------------------------------------
		* Load bms resource
		*/
	BmsHelper::LoadBmsOnThread(bms);
}

void ScenePlay::Update() {
	/* *******************************************************
	* check timers (game flow related)
	* *******************************************************/
	if (PLAYVALUE.OnReady->Trigger(PLAYVALUE.OnSongLoadingEnd->IsStarted() && PLAYVALUE.OnSongLoading->GetTick() >= 3000))
		PLAYVALUE.OnSongLoading->Stop();
	if (PLAYVALUE.OnSongStart->Trigger(PLAYVALUE.OnReady->GetTick() >= 1000))
		PLAYVALUE.OnSongLoadingEnd->Stop();
	// OnClose is called when all player is dead
	bool close = true;
	if (close && PLAYER[0] && PLAYER[0]->GetPlayerType() == PLAYERTYPE::HUMAN)
		close = close && PLAYER[0]->IsDead();
	if (close && PLAYER[1] && PLAYER[1]->GetPlayerType() == PLAYERTYPE::HUMAN)
		close = close && PLAYER[1]->IsDead();
	if (PLAYVALUE.OnClose->Trigger(close)) {
		// stop all sound
		SONGPLAYER->StopAllSound();
	}
	// OnFadeout is called when endtime is over
	// COMMENT: EndTime == lastnote + 2 sec.
	PLAYVALUE.OnFadeOut->Trigger(SONGPLAYER->GetEndTime() + 2000 < PLAYVALUE.OnGameStart->GetTick());
	// If OnClose/OnFadeout has enough time, then go to next scene
	if (PLAYVALUE.OnClose->GetTick() > 3000 || PLAYVALUE.OnFadeOut->GetTick() > 3000) {
		roundcnt = courseplay;
		round = round + 1;
		SCENE->ChangeScene("Result");
		return;
	}

	/* *********************************************************
	* under are part of playing
	* if dead, no need to update.
		* *********************************************************/
	if (PLAYVALUE.OnClose->IsStarted()) return;

	/*
		* BMS update
		*/
	if (PLAYVALUE.OnSongStart->IsStarted())
		SONGPLAYER->Update(PLAYVALUE.OnSongStart->GetTick());

	/*
		* Player update
		*/
	if (PLAYER[0]) PLAYER[0]->Update();
	if (PLAYER[1]) {
		PLAYER[1]->Update();
		// update rival score if P2 exists
		*pRivalDiff = 
			PLAYER[0]->GetScoreData()->CalculateEXScore() - 
			PLAYER[1]->GetScoreData()->CalculateEXScore();
		*pRivalDiff_d = PLAYER[1]->GetScoreData()->CurrentRate();
		*pRivalDiff_d_total = PLAYER[1]->GetScoreData()->CalculateRate();
	}
}

void ScenePlay::Render() {
	/*
		* draw skin tree
		*/
	theme.Update();
	theme.Render();
}

void ScenePlay::End() {
	/*
	* save player properties when game goes end
	*/
	PlayerInfoHelper::SavePlayerInfo(PLAYERINFO[0]);

	/*
	* if you hit note (not gave up) and recordable, then save record
	* (TODO)
	*/
	if (isrecordable && PLAYER[0]) {
		PLAYER[0]->Save();
	}
}

void ScenePlay::Release() {
	// remove player
	SAFE_DELETE(PLAYER[0]);
	SAFE_DELETE(PLAYER[1]);

	// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
	// we don't need to clear BMS data until next BMS is loaded
	theme.ClearElements();
	//bmsresource.Clear();
	//bms.Clear();
}

/** private */
int GetChannelFromFunction(int func) {
	int channel = -1;	// no channel
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
	case PlayerKeyIndex::P1_BUTTONSCUP:
		channel = 0;
		break;
	case PlayerKeyIndex::P1_BUTTONSCDOWN:
		channel = 8;
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
	case PlayerKeyIndex::P2_BUTTONSCUP:
		channel = 10;
		break;
	case PlayerKeyIndex::P2_BUTTONSCDOWN:
		channel = 18;
		break;
	case PlayerKeyIndex::P2_BUTTON9:
		channel = 19;
		break;
	}
	return channel;
}

// lane is changed instead of sudden if you press CTRL key
bool pressedCtrl = false;
bool pressedStart = false;
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
	int func = -1;
	switch (code) {
	case SDL_SCANCODE_F12:
		// refresh float speed and toggle floating mode
		PLAYER[0]->DeltaSpeed(0);
		PLAYERINFO[0].playconfig.usefloatspeed
			= !PLAYERINFO[0].playconfig.usefloatspeed;
		break;
	case SDL_SCANCODE_UP:
		// ONLY 1P. may need to PRS+WKEY if you need to control 2P.
		if (PLAYERINFO[0].playconfig.usefloatspeed)
			PLAYER[0]->DeltaFloatSpeed(0.001);
		else
			PLAYER[0]->DeltaSpeed(0.05);
		break;
	case SDL_SCANCODE_DOWN:
		// if pressed start button, float speed will change.
		if (PLAYERINFO[0].playconfig.usefloatspeed)
			PLAYER[0]->DeltaFloatSpeed(-0.001);
		else
			PLAYER[0]->DeltaSpeed(-0.05);
		break;
	case SDL_SCANCODE_RIGHT:
		if (pressedCtrl) PLAYER[0]->DeltaLift(0.05);
		else PLAYER[0]->DeltaSudden(0.05);
		break;
	case SDL_SCANCODE_LEFT:
		if (pressedCtrl) PLAYER[0]->DeltaLift(-0.05);
		else PLAYER[0]->DeltaSudden(-0.05);
		break;
	case SDL_SCANCODE_LCTRL:
		pressedCtrl = true;
		break;
	default:
		func = PlayerKeyHelper::GetKeyCodeFunction(KEYSETTING, code);
		switch (func) {
		case PlayerKeyIndex::P1_BUTTONSTART:
			SWITCH_ON("OnP1SuddenChange");
			pressedStart = true;
			break;
		default:
		{
			// change it to key channel ...
			int channel = GetChannelFromFunction(func);
			if (channel >= 0) {
				if (PLAYER[0]) PLAYER[0]->PressKey(channel);
				if (PLAYER[1]) PLAYER[1]->PressKey(channel);
			}
		}
		}
	}
}

void ScenePlay::KeyUp(int code) {
	int func = -1;
	switch (code) {
	case SDLK_LCTRL:
		pressedCtrl = false;
		break;
	default:
		func = PlayerKeyHelper::GetKeyCodeFunction(KEYSETTING, code);
		switch (func) {
		case PlayerKeyIndex::P1_BUTTONSTART:
			SWITCH_OFF("OnP1SuddenChange");
			pressedStart = false;
			break;
		default:
		{
			int channel = GetChannelFromFunction(func);
			if (channel >= 0) {
				if (PLAYER[0]) PLAYER[0]->UpKey(channel);
				if (PLAYER[1]) PLAYER[1]->UpKey(channel);
			}
		}
		}
	}
}