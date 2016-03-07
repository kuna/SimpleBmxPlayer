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



void ScenePlay::Initialize() {
	OnReady = SWITCH_OFF("OnReady");
	OnClose = SWITCH_OFF("OnClose");
	OnFadeIn = SWITCH_OFF("OnFadeIn");
	OnFadeOut = SWITCH_OFF("OnFadeOut");

	m_DiffSwitch[0].SetFromPool("DiffUnknown");
	m_DiffSwitch[1].SetFromPool("DiffBeginner");
	m_DiffSwitch[2].SetFromPool("DiffNormal");
	m_DiffSwitch[3].SetFromPool("DiffHyper");
	m_DiffSwitch[4].SetFromPool("DiffAnother");
	m_DiffSwitch[5].SetFromPool("DiffInsane");
}

void ScenePlay::Start() {
	//
	// set scene state from game state
	//
	// - if autoplay | replay | rate < 1 | startmeasure != 0 | endmeasure < 1000
	// then don't allow save play record.
	if (GAMESTATE.m_Autoplay || GAMESTATE.m_Replay || GAMESTATE.m_PlayRate < 1.0
		|| GAMESTATE.m_Startmeasure != 0 || GAMESTATE.m_Endmeasure < 1000 || GAMESTATE.m_SongRepeatCount > 1)
	{
		m_IsRecordable = false;
	}
	else {
		m_IsRecordable = true;
	}

	//
	// load bms
	// (with parameter settings)
	//
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

	// COMMENT: set player gauge if it's coursemode ?



	//
	// bms note should be created (bms & status requires..?)
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

	for (int i = 0; i < 6; i++) {
		if (songinfo.iDifficulty == i)
			m_DiffSwitch[i].Start();
		else
			m_DiffSwitch[i].Stop();
	}
	m_PlayLevel = songinfo.iLevel;

	if (songinfo.sBackBmp.size()) {
		// TODO: load backbmp
		SWITCH_ON("IsBACKBMP");
	}
	else {
		SWITCH_OFF("IsBACKBMP");
	}



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
	// loading -> ready -> play
	if (OnSongLoadingEnd->IsStarted() && OnSongLoading->GetTick() >= m_MinLoadingTime) {
		OnSongLoading->Stop();
		OnReady->Start();
	}
	if (OnReady->GetTick() >= m_ReadyTime) {
		OnReady->Stop();
		SONGPLAYER->Play();
	}

	// all human player is dead -> OnClose
	bool close = true;
	if (close && PLAYER[0] && PLAYER[0]->GetPlayerType() == PLAYERTYPE::HUMAN)
		close = close && PLAYER[0]->IsDead();
	if (close && PLAYER[1] && PLAYER[1]->GetPlayerType() == PLAYERTYPE::HUMAN)
		close = close && PLAYER[1]->IsDead();
	if (OnClose->Trigger(close)) {
		// stop all sound
		SONGPLAYER->StopAllSound();
	}

	// OnFadeout is called when endtime is over
	// COMMENT: EndTime == lastnote + 2 sec.
	OnFadeOut->Trigger(SONGPLAYER->GetEndTime() + 2000 < OnSongStart->GetTick());
	// If OnClose/OnFadeout has enough time, then go to next scene
	if (OnClose->GetTick() > 3000 || OnFadeOut->GetTick() > 3000) {
		roundcnt = courseplay;
		round = round + 1;
		SCENE->ChangeScene("Result");
		return;
	}

	/* *********************************************************
	* under are part of playing
	* if dead, no need to update.
	* *********************************************************/
	if (OnClose->IsStarted()) return;


	/*
	* BMS update
	*/
	if (OnSongStart->IsStarted())
		SONGPLAYER->Update(OnSongStart->GetTick());

	/*
	* Player update
	*/
	if (PLAYER[0]) PLAYER[0]->Update();
	if (PLAYER[1]) PLAYER[1]->Update();
	// update rival score
	*pRivalDiff = 
		PLAYER[0]->GetScoreData()->CalculateEXScore() - 
		PLAYER[1]->GetScoreData()->CalculateEXScore();
	*pRivalDiff_d = PLAYER[1]->GetScoreData()->CurrentRate();
	*pRivalDiff_d_total = PLAYER[1]->GetScoreData()->CalculateRate();
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