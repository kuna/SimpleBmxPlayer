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
	vTargetDiff.SetFromPool("RivalDiff");	// TargetDiff

	for (int i = 0; i < 10; i++)
		OnCourseRound[i].SetFromPool("Round" + i);
	OnDemo.SetFromPool("Demo");
	OnExpert.SetFromPool("Expert");
	OnCourse.SetFromPool("Course");
	OnGrade.SetFromPool("Grade");
}

void ScenePlay::Start() {
	//
	// Initalize global objects from SETTING
	//
	/* (rseed, repeat/start/end, rate, etc ...) */
	int rseed = SETTING->m_seed;	// TODO: save it as global to use it as replay data ...?
	if (rseed < 0) rseed = time(0) % 65536;

	if (SETTING->trainingmode) {
		BmsHelper::SetBeginMeasure(SETTING->startmeasure);
		BmsHelper::SetEndMeasure(SETTING->endmeasure);
		BmsHelper::SetRepeat(SETTING->repeat);
		BmsHelper::SetTrainingmode(true);
	}
	else {
		BmsHelper::SetTrainingmode(false);
	}

	BmsHelper::SetBgaChannel(SETTING->bga);

	//
	// get current song path
	//
	m_Songpath = SETTING->m_CoursePath[SETTING->m_CourseRound];
	m_Songhash = SETTING->m_CourseHash[SETTING->m_CourseRound];


	//
	// load bms
	// (with parameter settings)
	//
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
	/* initalize gauge */
	// if it's first round, then set player gauge as initialized one
	// otherwise, use previous value of stored in scene.
	if (SETTING->m_CourseRound == 0) {
		// set initialized gauge (TODO)
		m_Playerhealth[0]
			= m_Playerhealth[1]
			= m_Playerhealth[2] = 1.0;
	}

	/* parse pacemaker/main player information */
	int playerside = 0;		// main playerside (decide main scoregraph)
	if (!PROFILE[0]->IsProfileLoaded()) playerside = 1;
	int pacemaker_type = PROFILE[playerside]->config.pacemaker_type;
	int pacemaker_goal = PROFILE[playerside]->config.pacemaker_goal;

	for (int i = 0; i < 1; i++) {
		if (PROFILE[i]->IsProfileLoaded()) {
			// make human player
			PLAYER[i] = new Player(i);
		}
		else {
			// make this as pacemaker
			if (pacemaker_type == PACEMAKERTYPE::PACEMYBEST) {
				// make pacemaker as replay
				ReplayData rep;
				if (!PROFILE[i]->LoadReplayData(m_Songhash, rep)) {
					LOG->Critical("Failed to load replay file.");
					return;		// ??
				}
				PlayerReplay *pRep = new PlayerReplay(i);
				rep.SetRound(SETTING->m_CourseRound);
				pRep->SetReplay(rep);
				PLAYER[i] = pRep;
				break;
			}
			else {
				// autoplay (pacemaker)
				PlayerAuto* p = new PlayerAuto(i);
				p->SetGoal(pacemaker_goal / 100.0f);
				p->SetPlaySound(false);
				PLAYER[i] = p;
				break;
			}
		}

		// set basic gauge state
		PLAYER[i]->SetGauge(m_Playerhealth[i]);
	}

	/* generate MYBEST player */
	PlayerReplay *p_mybest = new PlayerReplay(2);
	// load player record
	PlayRecord record;
	if (PROFILE[0]->LoadSongRecord(m_Songhash, record))
	{
		// theme metrics is automatically set, so we don't need to care.

		// set replay data for P2(MYBEST)
		ReplayData rep;
		if (PROFILE[0]->LoadReplayData(m_Songhash, rep))
			p_mybest->SetReplay(rep);
	}
	PLAYER[2] = p_mybest;
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

	OnDemo.Stop();
	OnExpert.Stop();
	OnCourse.Stop();
	OnGrade.Stop();
	switch (SETTING->gamemode) {
	case GAMEMODE_DEMO:
		OnDemo.Start();
		break;
	case GAMEMODE_EXPERT:
		OnExpert.Start();
		break;
	case GAMEMODE_COURSE:
		OnCourse.Start();
		break;
	case GAMEMODE_GRADE:
		OnGrade.Start();
		break;
	}
	for (int i = 0; i < 10; i++) {
		if (i == SETTING->m_CourseRound)
			OnCourseRound[i].Start();
		else
			OnCourseRound[i].Stop();
	}


	/* ---------------------------------------------------------------------------
	* Load skin
	* - from here, something will show up in the screen.
	*/
	RString PlayskinPath = "";
	if (sinfo.iKeyCount < 10)
		PlayskinPath = SETTING->skin_play_7key;
	else
		PlayskinPath = SETTING->skin_play_14key;
	if (!theme.Load(PlayskinPath))
		LOG->Critical("Failed to load play skin: %s", PlayskinPath.c_str());



	/* ---------------------------------------------------------------------------
	 * Load bms resource
	 */
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
		SETTING->m_CourseRound += 1;
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
	vTargetDiff =
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
	// Theme/Pool will automatically release all texture resources.
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
		PLAYER[0]->RefreshFloatSpeed();
		break;
	case SDL_SCANCODE_UP:
		// ONLY 1P. may need to PRS+WKEY if you need to control 2P.
		if (PROFILE[0]->option.usefloatspeed)
			PLAYER[0]->DeltaFloatSpeed(0.001);
		else
			PLAYER[0]->DeltaSpeed(0.05);
		break;
	case SDL_SCANCODE_DOWN:
		// if pressed start button, float speed will change.
		if (PROFILE[0]->option.usefloatspeed)
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
		func = KEYSETTING->GetKeyCodeFunction(code);
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
		func = KEYSETTING->GetKeyCodeFunction(code);
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