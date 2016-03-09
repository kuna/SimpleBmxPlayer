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
#include "Player.h"
#include "Profile.h"
#include "Setting.h"
#include "game.h"
#include "Sceneresult.h"



void ScenePlay::Initialize() {
	OnReady.SetFromPool("Ready");
	OnClose.SetFromPool("Close");

	// COMMENT: option related flag ...?
	SWITCH_ON("IsScoreGraph");
	SWITCH_ON("IsBGA");
	SWITCH_ON("IsExtraMode");
	SWITCH_ON("IsCourseMode");
}

void ScenePlay::Start() {
	//
	// get current song path
	//
	m_Songpath = GAMESTATE.m_CoursePath[GAMESTATE.m_CourseRound];
	m_Songhash = GAMESTATE.m_CourseHash[GAMESTATE.m_CourseRound];


	//
	// load bms
	// (with parameter settings)
	//
	BmsHelper::SetLoadOption(
		GAMESTATE.m_Startmeasure,
		GAMESTATE.m_Endmeasure,
		GAMESTATE.m_SongRepeatCount,
		GAMESTATE.m_ShowBga
		);
	BmsBms bms;
	if (BmsHelper::LoadBms(m_Songpath, bms)) {
		LOG->Info("BMS loading finished successfully (%s)\n", m_Songpath.c_str());
	}
	else {
		LOG->Critical("BMS loading failed (%s)\n", m_Songpath.c_str());
	}



	//
	// create / initalize players
	//
	for (int i = 0; i < 2; i++) {
		switch (GAMESTATE.m_Player[i].playertype) {
		case PLAYERTYPE::HUMAN:
			PLAYER[i] = new Player(i);
			break;
		case PLAYERTYPE::AUTO:
		case PLAYERTYPE::NETWORK:
			// set pacemaker, and make silent
			PlayerAuto* p = new PlayerAuto(i);
			p->SetGoal(GAMESTATE.m_PacemakerGoal);
			p->SetPlaySound(false);
			PLAYER[i] = p;
			break;
		case PLAYERTYPE::REPLAY:
			// set replay if necessary
			ReplayData rep;
			if (!ReplayHelper::LoadReplay(rep, PLAYERINFO[i].name, m_Songhash)) {
				LOG->Critical("Failed to load replay file.");
				return;		// ??
			}
			PlayerReplay *pRep = new PlayerReplay(i);
			rep.SetRound(0);
			pRep->SetReplay(rep);
			PLAYER[i] = pRep;
			break;
		}

		// set basic gauge state
		// TODO
	}

	// generate replay object (MYBEST)
	PlayerReplay *p_mybest = new PlayerReplay(2);
	// load player record
	PlayerSongRecord record;
	if (PlayerRecordHelper::LoadPlayerRecord(
		record, PLAYERINFO[0].name, m_Songhash))
	{
		// set is previously cleared? state.
		// TODO: that should be in playerinfo status.
		INTPOOL->Set("SongClear", record.status);

		// set replay
		ReplayData rep;
		ReplayHelper::LoadReplay(
			rep, PLAYERINFO[0].name, m_Songhash);
		p_mybest->SetReplay(rep);
	}
	PLAYER[2] = p_mybest;

	// if it's first round, then set player gauge as initialized one
	// otherwise, use previous value of stored in scene.
	if (GAMESTATE.m_CourseRound == 0) {
		// set initialized gauge (TODO)
		m_Playerhealth[0]
			= m_Playerhealth[1]
			= m_Playerhealth[2] = 1.0;
	}
	PLAYER[0]->SetGauge(m_Playerhealth[0]);
	PLAYER[1]->SetGauge(m_Playerhealth[1]);
	PLAYER[2]->SetGauge(m_Playerhealth[2]);



	//
	// bms note should be created (bms & status requires..?)
	//
	if (PLAYER[0]) PLAYER[0]->InitalizeNote(&bms);
	if (PLAYER[1]) PLAYER[1]->InitalizeNote(&bms);	// COMMENT: player 2 may use different bms file if is battle mode ...
	if (PLAYER[2]) PLAYER[2]->InitalizeNote(&bms);



	/* ---------------------------------------------------------------------------
	* Set rendervalue/switches before skin is loaded
	*/
	OnReady.Stop();
	OnClose.Stop();
	SongInfo sinfo;
	BmsHelper::GetBmsMetadata(bms, sinfo);
	SONGMANAGER->SetMetrics(sinfo);			// set theme metrics


	/* ---------------------------------------------------------------------------
	* Load skin
	* - from here, something will show up in the screen.
	*/
	RString PlayskinPath = "";
	if (sinfo.iKeyCount < 10)
		PlayskinPath = SETTING.skin_play_7key;
	else
		PlayskinPath = SETTING.skin_play_14key;
	if (!theme.Load(PlayskinPath))
		LOG->Critical("Failed to load play skin: %s", PlayskinPath.c_str());



	/* ---------------------------------------------------------------------------
	 * Load bms resource
	 */
	SONGPLAYER->SetRate(GAMESTATE.m_PlayRate);
	SONGPLAYER->SetMinLoadingTime(m_MinLoadingTime);
	FILEMANAGER->PushBasePath(m_Songpath);		// COMMENT: is it safe???
	BmsHelper::LoadBmsOnThread(bms);
}

void ScenePlay::Update() {
	/* *******************************************************
	* check timers (game flow related)
	* *******************************************************/
	// loading -> ready -> play
	OnReady.Trigger(SONGPLAYER->IsBmsLoaded());
	if (OnReady.GetTick() >= m_ReadyTime) {
		OnReady.Stop();
		SONGPLAYER->Play();
	}

	// all human player is dead -> OnClose
	bool close = (PLAYER[0]->IsHuman() && PLAYER[0]->IsDead())
		&& (PLAYER[1]->IsHuman() && PLAYER[1]->IsDead());
	if (OnClose.Trigger(close)) {
		// stop all sound
		SONGPLAYER->StopAllSound();
	}

	// If dead / song end, then goto next scene.
	if (OnClose.GetTick() > 3000 || SONGPLAYER->GetTick() > SONGPLAYER->GetEndTime() + 2000) {
		GAMESTATE.m_CourseRound += 1;
		SCENE->ChangeScene("Result");
		return;
	}

	/* don't update player or Bms If dead. */
	if (OnClose.IsStarted()) return;


	/*
	 * BMS update
	 */
	if (SONGPLAYER->IsPlaying())
		SONGPLAYER->Update();

	/*
	 * Player update
	 */
	for (int i = 0; i < 2; i++) {
		PLAYER[i]->Update();
	}
	// update rival score
	vRivalDiff =
		PLAYER[0]->GetScoreData()->CalculateEXScore() -
		PLAYER[1]->GetScoreData()->CalculateEXScore();
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
	* if you hit note (not gave up) and recordable, then save record/replay
	*/
	if (PLAYER[0]->IsSaveable()) PLAYER[0]->Save();
	if (PLAYER[1]->IsSaveable()) PLAYER[1]->Save();
	//if (isrecordable && !m_Giveup)

	/*
	* Store all player information to game state
	* (TODO)
	*/

	/*
	* done, release all players.
	*/
	if (PLAYER[0]) SAFE_DELETE(PLAYER[0]);	// player
	if (PLAYER[1]) SAFE_DELETE(PLAYER[1]);	// 2p(battlemode) or auto-player(pacemaker)
	if (PLAYER[2]) SAFE_DELETE(PLAYER[2]);	// replay(mybest)

	// skin clear (COMMENT: we don't need to release skin in real. just in beta version.)
	// we don't need to clear BMS data until next BMS is loaded
	theme.ClearElements();
	//bmsresource.Clear();
	//bms.Clear();
}

void ScenePlay::Release() {
	// Nothing to do, maybe.
	// Theme/Pool will automaticall release all texture resources.
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
void ScenePlay::OnDown(int code) {
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

void ScenePlay::OnUp(int code) {
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