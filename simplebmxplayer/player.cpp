#include "player.h"
#include "global.h"
#include "handlerargs.h"
#include "bmsresource.h"
#include "skinrendertree.h"
#include "util.h"
#include <time.h>

// global
Player*		PLAYER[4];

// macro
#define PLAYSOUND(k)	BmsResource::SOUND.Play(k)

/* constant value - used for rendering & initalizing */
namespace {
	const int lane_to_channel[] = {
		6, 1, 2, 3, 4, 5, 8, 9, 6, 7,
		16, 11, 12, 13, 14, 15, 16, 18, 19, 17,
	};
	const int channel_to_lane[] = {
		0, 1, 2, 3, 4, 5, 0, 9, 6, 7,
		0, 1, 2, 3, 4, 5, 0, 9, 6, 7,
	};
}

// ------- Player ----------------------------------------

Player::Player(PlayerPlayConfig* config, BmsNoteContainer *note, 
	int playside, int playmode, int playertype) {
	this->playconfig = config;
	this->playside = playside;
	this->playmode = playmode;
	this->playertype = playertype;

	// initalize basic variables/timers
	pBmstimer = TIMERPOOL->Get("OnGameStart");
	pExscore_graph = DOUBLEPOOL->Get("ExScore");
	pHighscore_graph = DOUBLEPOOL->Get("HighScore");
	pOn1pmiss = TIMERPOOL->Get("On1PMiss");
	pOn2pmiss = TIMERPOOL->Get("On2PMiss");
	pOn1pjudge = TIMERPOOL->Get("On1PJudge");
	pOn2pjudge = TIMERPOOL->Get("On2PJudge");
	if (playside == 0) {
		pPlayergauge = DOUBLEPOOL->Get("Player1Gauge");
		pPlayergaugetype = INTPOOL->Get("Player1GaugeType");
		pPlaygroovegauge = INTPOOL->Get("Play1PGrooveGuage");
		pPlayscore = INTPOOL->Get("Play1PScore");
		pPlayexscore = INTPOOL->Get("Play1PExScore");
		pPlaycombo = INTPOOL->Get("Play1PCombo");
		pPlaymaxcombo = INTPOOL->Get("Play1PMaxCombo");
		pPlaytotalnotes = INTPOOL->Get("Play1PTotalNotes");
		pPlayrivaldiff = INTPOOL->Get("Play1PRivalDiff");
		pOnfullcombo = TIMERPOOL->Get("On1PFullCombo");
		pOnlastnote = TIMERPOOL->Get("On1PLastNote");
		pOnGameover = TIMERPOOL->Get("On1PGameOver");
		pOnGaugeMax = TIMERPOOL->Get("On1PGaugeMax");
		pOnJudge[5] = TIMERPOOL->Set("On1PJudgePerfect");
		pOnJudge[4] = TIMERPOOL->Set("On1PJudgeGreat");
		pOnJudge[3] = TIMERPOOL->Set("On1PJudgeGood");
		pOnJudge[2] = TIMERPOOL->Set("On1PJudgeBad");
		pOnJudge[1] = TIMERPOOL->Set("On1PJudgePoor");
		pOnJudge[0] = TIMERPOOL->Set("On1PJudgePoor");
		pNoteSpeed = INTPOOL->Get("1PSpeed");
		pFloatSpeed = INTPOOL->Get("1PFloatSpeed");
		pSuddenHeight = INTPOOL->Get("1PSudden");
		pLiftHeight = INTPOOL->Get("1PLift");
	}
	else {
		pPlayergauge = DOUBLEPOOL->Get("Player2Gauge");
		pPlayergaugetype = INTPOOL->Get("Player2GaugeType");
		pPlaygroovegauge = INTPOOL->Get("Play2PGrooveGuage");
		pPlayscore = INTPOOL->Get("Play2PScore");
		pPlayexscore = INTPOOL->Get("Play2PExScore");
		pPlaycombo = INTPOOL->Get("Play2PCombo");
		pPlaymaxcombo = INTPOOL->Get("Play2PMaxCombo");
		pPlaytotalnotes = INTPOOL->Get("Play2PTotalNotes");
		pPlayrivaldiff = INTPOOL->Get("Play2PRivalDiff");
		pOnfullcombo = TIMERPOOL->Get("On2PFullCombo");
		pOnlastnote = TIMERPOOL->Get("On2PLastNote");
		pOnGameover = TIMERPOOL->Get("On2PGameOver");
		pOnGaugeMax = TIMERPOOL->Get("On2PGaugeMax");
		pOnJudge[5] = TIMERPOOL->Set("On2PJudgePerfect");
		pOnJudge[4] = TIMERPOOL->Set("On2PJudgeGreat");
		pOnJudge[3] = TIMERPOOL->Set("On2PJudgeGood");
		pOnJudge[2] = TIMERPOOL->Set("On2PJudgeBad");
		pOnJudge[1] = TIMERPOOL->Set("On2PJudgePoor");
		pOnJudge[0] = TIMERPOOL->Set("On2PJudgePoor");
		pNoteSpeed = INTPOOL->Get("2PSpeed");
		pFloatSpeed = INTPOOL->Get("2PFloatSpeed");
		pSuddenHeight = INTPOOL->Get("2PSudden");
		pLiftHeight = INTPOOL->Get("2PLift");
	}

	memset(pLanepress, 0, sizeof(pLanepress));
	memset(pLaneup, 0, sizeof(pLaneup));
	memset(pLanehold, 0, sizeof(pLanehold));
	memset(pLanejudgeokay, 0, sizeof(pLanejudgeokay));
	for (int i = 0; i < 20; i++) {
		int player = i / 10 + 1;
		if (i == 6) {
			pLanepress[i] = TIMERPOOL->Set(ssprintf("On%dPKeySCPress", player), false);
			pLaneup[i] = TIMERPOOL->Set(ssprintf("On%dPKeySCUp", player), false);
			pLanehold[i] = TIMERPOOL->Set(ssprintf("On%dPJudgeSCHold", player), false);
			pLanejudgeokay[i] = TIMERPOOL->Set(ssprintf("On%dPJudgeSCOkay", player), false);
		}
		else {
			pLanepress[i] = TIMERPOOL->Set(ssprintf("On%dPKey%dPress", player, channel_to_lane[i]), false);
			pLaneup[i] = TIMERPOOL->Set(ssprintf("On%dPKey%dUp", player, channel_to_lane[i]), false);
			pLanehold[i] = TIMERPOOL->Set(ssprintf("On%dPJudge%dHold", player, channel_to_lane[i]), false);
			pLanejudgeokay[i] = TIMERPOOL->Set(ssprintf("On%dPJudge%dOkay", player, channel_to_lane[i]), false);
		}
		noteindex[i] = -1;
	}

	// initialize gauge
	playergaugetype = config->gaugetype;
	switch (playergaugetype) {
	case GAUGETYPE::GROOVE:
	case GAUGETYPE::EASY:
	case GAUGETYPE::ASSISTEASY:
		SetGauge(0.2);
	default:
		SetGauge(1.0);
	}

	// initalize note/score
	bmsnote = note;
	memset(&score, 0, sizeof(score));
	score.totalnote = bmsnote->GetNoteCount();
	judgenotecnt = 0;
	for (int i = 0; i < 20; i++) {
		noteindex[i] = GetAvailableNoteIndex(i);
	}

	// initalize note speed
	speed_mul = 1;
	switch (playconfig->speedtype) {
	case SPEEDTYPE::MAXBPM:
		speed_mul = 120.0 / BmsHelper::GetMaxBPM();
		break;
	case SPEEDTYPE::MINBPM:
		speed_mul = 120.0 / BmsHelper::GetMinBPM();
		break;
	case SPEEDTYPE::MEDIUM:
		speed_mul = 120.0 / BmsHelper::GetMediumBPM();
		break;
	}
	if (playconfig->usefloatspeed) {
		SetFloatSpeed(playconfig->floatspeed);
	}
	else {
		SetSpeed(playconfig->speed);
	}
}

void Player::SetGauge(double v) {
	if (v >= 1) {
		pOnGaugeMax->Trigger();
		v = 1;
	}
	else {
		pOnGaugeMax->Stop();
	}
	if (v < 0) {
		pOnGameover->Trigger();
		v = 0;
	}
	*pPlayergauge = playergauge = v;
	*pPlaygroovegauge = (int)(v * 50) * 2;
}

namespace {
	double ConvertSpeedToFloat(double speed, double bpm) {
		/*
		 * the time 1 beat needs to go off:
		 * 1 / (speed) / (BPM) * 60 (sec)
		 */
		return 1.0 / speed / bpm * 60 * 4;
	}

	double ConvertFloatToSpeed(double speed, double bpm) {
		/*
		 * just reverse equation
		 * 1 / (float) / (BPM) * 60 (speed) 
		 */
		return 1.0 / speed / bpm * 60 * 4;
	}
}

void Player::SetSpeed(double speed) {
	/*
	 * set basic value
	 */
	*pNoteSpeed = speed * 100;
	playconfig->speed
		= notespeed
		= speed;
	playconfig->floatspeed
		= notefloat
		= ConvertSpeedToFloat(speed, BmsHelper::GetCurrentBPM());

	/*
	 * set the pointer values
	 */
	*pNoteSpeed = notespeed * 100;
	*pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pSuddenHeight = notefloat * suddenheight;
	*pLiftHeight = notefloat * liftheight;
}
void Player::DeltaSpeed(double speed) { SetSpeed(notespeed + speed); }

void Player::SetFloatSpeed(double speed) {
	double _s = ConvertFloatToSpeed(speed, BmsHelper::GetCurrentBPM());
	playconfig->speed
		= notespeed
		= _s;
	playconfig->floatspeed
		= notefloat
		= speed;

	/*
	 * set the pointer values
	 */
	*pNoteSpeed = notespeed * 100;
	*pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pSuddenHeight = notefloat * suddenheight;
	*pLiftHeight = notefloat * liftheight;
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(notefloat + speed); }

void Player::SetSudden(double height) {
	playconfig->sudden
		= suddenheight
		= height;

	/*
	 * set the pointer values
	 */
	*pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pSuddenHeight = notefloat * suddenheight;
	*pLiftHeight = notefloat * liftheight;
}
void Player::DeltaSudden(double height) { SetSudden(suddenheight + height); }

void Player::SetLift(double height) {
	playconfig->lift
		= liftheight
		= height;

	/*
	 * set the pointer values
	 */
	*pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pSuddenHeight = notefloat * suddenheight;
	*pLiftHeight = notefloat * liftheight;
}
void Player::DeltaLift(double height) { SetLift(liftheight + height); }

int Player::CheckJudgeByTiming(double delta) {
	double abs_delta = abs(delta);
	if (abs_delta <= BmsJudgeTiming::PGREAT)
		return JUDGETYPE::JUDGE_PGREAT;
	else if (abs_delta <= BmsJudgeTiming::GREAT)
		return JUDGETYPE::JUDGE_GREAT;
	else if (abs_delta <= BmsJudgeTiming::GOOD)
		return JUDGETYPE::JUDGE_GOOD;
	else if (abs_delta <= BmsJudgeTiming::BAD)
		return JUDGETYPE::JUDGE_BAD;
	else if (abs_delta <= BmsJudgeTiming::POOR)
		return JUDGETYPE::JUDGE_POOR;
	else if (abs_delta <= BmsJudgeTiming::NPOOR)
		return JUDGETYPE::JUDGE_NPOOR;
	else if (delta < 0)
		return JUDGETYPE::JUDGE_LATE;
	else
		return JUDGETYPE::JUDGE_EARLY;
}

void Player::MakeJudge(int judgetype, int channel, bool silent) {
	score.AddGrade(judgetype);
	if (judgetype >= JUDGETYPE::JUDGE_GREAT && pLanejudgeokay[channel])
		pLanejudgeokay[channel]->Start();
	// update graph/number
	if (judgetype <= JUDGETYPE::JUDGE_BAD)
		pOn1pmiss->Start();
	// update judged note count
	switch (judgetype) {
	case JUDGETYPE::JUDGE_POOR:
	case JUDGETYPE::JUDGE_BAD:
	case JUDGETYPE::JUDGE_GOOD:
	case JUDGETYPE::JUDGE_GREAT:
	case JUDGETYPE::JUDGE_PGREAT:
		judgenotecnt++;
		break;
	}
	// update gauge
	SetGauge(playergauge + 0.01);
	// update exscore/combo/etc ...
	*pExscore_graph = score.CalculateRate();
	*pHighscore_graph = score.CalculateRate();
	*pPlayscore = score.CalculateScore();
	*pPlayexscore = score.CalculateEXScore();
	*pPlaycombo = score.combo;
	*pPlaymaxcombo = score.maxcombo;
	// fullcombo/lastnote check
	if (score.combo == score.totalnote)
		pOnfullcombo->Start();
	if (judgenotecnt == score.totalnote)
		pOnlastnote->Start();

	if (!silent) {
		// show judge(combo) element
		for (int i = 0; i < 6; i++) {
			if (i == judgetype)
				pOnJudge[i]->Start();
			else
				pOnJudge[i]->Stop();
		}
		// judge timer start
		// consider DP also.
		if (playmode >= 10) {
			// DP
			if (channel < 10) pOn1pjudge->Start();
			else pOn2pjudge->Start();
		}
		else {
			// SP
			if (playside == 0) pOn1pjudge->Start();
			else pOn2pjudge->Start();
		}
		
		// TODO: make judge event
		//Handler::CallHandler(OnGamePlayJudge, &judgearg);
	}
}

bool Player::IsNoteAvailable(int notechannel) {
	return (noteindex[notechannel] >= 0);
}

int Player::GetAvailableNoteIndex(int notechannel, int start) {
	// we also ignore invisible note!
	for (int i = start; i < (*bmsnote)[notechannel].size(); i++)
		if ((*bmsnote)[notechannel][i].type != BmsNote::NOTE_NONE 
			&& (*bmsnote)[notechannel][i].type != BmsNote::NOTE_HIDDEN)
			return i;
	return -1;	// cannot find next available note
}

int Player::GetNextAvailableNoteIndex(int notechannel) {
	if (!IsNoteAvailable(notechannel))
		return -1;
	return GetAvailableNoteIndex(notechannel, noteindex[notechannel] + 1);
}

BmsNote* Player::GetCurrentNote(int notechannel) {
	if (!IsNoteAvailable(notechannel))
		return 0;
	return &(*bmsnote)[notechannel][noteindex[notechannel]];
}

bool Player::IsLongNote(int notechannel) {
	return longnotestartpos[notechannel] > -1;
}

//
// game flow
//
void Player::Reset() {
	// recalculate note index/bgm index/position
	currentbar = BmsHelper::GetCurrentBar();

	// reset each of the note index
	for (int i = 0; i < 20; i++) {
		noteindex[i] = GetAvailableNoteIndex(i, currentbar);
	}
}

void Player::Update() {
	// timer/value update
	pOn1pjudge->OffTrigger(pOn1pjudge->GetTick() > 500);
	pOn2pjudge->OffTrigger(pOn2pjudge->GetTick() > 500);
	pOn1pmiss->OffTrigger(pOn1pmiss->GetTick() > 1000);
	pOn2pmiss->OffTrigger(pOn2pmiss->GetTick() > 1000);

	// if gameover then ignore
	if (IsDead()) return;

	// get time
	currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	currentbar = BmsHelper::GetCurrentBar();

	// check for note judgement
	for (int i = 0; i < 20; i++) {
		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (IsNoteAvailable(i) && noteindex[i] <= currentbar) {
			// if note is mine, then lets ignore as fast as we can
			if (GetCurrentNote(i)->type == BmsNote::NOTE_MINE) {
				// ignore the note
			}
			// if not autoplay, check timing for poor
			else if (CheckJudgeByTiming(BmsHelper::GetCurrentTimeFromBar(noteindex[i]) - time_sec) == JUDGETYPE::JUDGE_LATE) {
				BmsNote& note = (*bmsnote)[i][noteindex[i]];
				// if LNSTART, also kill LNEND & 2 miss
				if (note.type == BmsNote::NOTE_LNSTART) {
					note.type = BmsNote::NOTE_NONE;
					noteindex[i] = GetNextAvailableNoteIndex(i);
					MakeJudge(JUDGETYPE::JUDGE_POOR, i);
				}
				// if LNEND, reset longnotestartpos & remove LNSTART note
				else if (note.type == BmsNote::NOTE_LNEND) {
					(*bmsnote)[i][longnotestartpos[i]].type = BmsNote::NOTE_NONE;
					longnotestartpos[i] = -1;
				}
				note.type = BmsNote::NOTE_NONE;
				// make judge
				// (CLAIM) if hidden note isn't ignored by GetNextAvailableNoteIndex(), 
				// you have to hit it or you'll get miss.
				MakeJudge(JUDGETYPE::JUDGE_POOR, i);
			}

			// retrieve next note
			noteindex[i] = GetNextAvailableNoteIndex(i);
		}
	}
}

// key input
void Player::UpKey(int keychannel) {
	// if gameover then ignore
	if (IsDead()) return;
	// make judge (if you're pressing longnote)
	if (IsLongNote(keychannel)) {
		if (IsNoteAvailable(keychannel) && GetCurrentNote(keychannel)->type == BmsNote::NOTE_LNEND) {
			double t = currenttime / 1000.0;
			// make judge
			int judge = CheckJudgeByTiming(t - BmsHelper::GetCurrentTimeFromBar(noteindex[keychannel]));
			MakeJudge(judge, keychannel);
			// get next note and remove current longnote
			(*bmsnote)[keychannel][longnotestartpos[keychannel]].type = BmsNote::NOTE_NONE;
			(*bmsnote)[keychannel][noteindex[keychannel]].type = BmsNote::NOTE_NONE;
			noteindex[keychannel] = GetNextAvailableNoteIndex(keychannel);
		}
		longnotestartpos[keychannel] = -1;
	}
}

void Player::PressKey(int keychannel) {
	// if gameover then ignore
	if (IsDead()) return;
	// make judge
	if (IsNoteAvailable(keychannel)) {
		double t = currenttime / 1000.0;
		int judge = CheckJudgeByTiming(t - BmsHelper::GetCurrentTimeFromBar(noteindex[keychannel]));
		if (GetCurrentNote(keychannel)->type == BmsNote::NOTE_LNSTART) {
			// store longnote start pos & set longnote status
			MakeJudge(judge, keychannel, true);
			longnotestartpos[keychannel] = noteindex[keychannel];
		}
		else if (GetCurrentNote(keychannel)->type == BmsNote::NOTE_MINE) {
			// damage, ouch
			if (GetCurrentNote(keychannel)->value == BmsWord::MAX)
				health = 0;
			else
				health -= GetCurrentNote(keychannel)->value.ToInteger();
		}
		else if (GetCurrentNote(keychannel)->type == BmsNote::NOTE_NORMAL) {
			MakeJudge(judge, keychannel);
		}
	}
	PLAYSOUND(keysound[keychannel][currentbar].ToInteger());
}

double Player::GetLastMissTime() {
	// TODO
	return 0;
}

int Player::GetCurrentNoteBar(int channel) {
	return noteindex[channel];
}

bool Player::IsDead() {
	return pOnGameover->IsStarted();
}

void Player::RenderNote(SkinPlayObject *playobj) {
	double currentpos = BmsHelper::GetCurrentPosFromTime(currenttime / 1000.0);
	// TODO: draw LINE/JUDGELINE

	// in lane, 0 == SC
	// we have to check lane to ~ 20, actually.
	// COMMENT: maybe we have to convert lane number into channel number?
	double lnpos[20] = { 0, };
	bool lnstart[20] = { false, };
	for (int lane = 0; lane <= 7; lane++) {
		int channel = lane_to_channel[lane];
		int currentnotebar = GetCurrentNoteBar(channel);
		while (currentnotebar >= 0) {
			double pos = BmsHelper::GetCurrentPosFromBar(currentnotebar) - currentpos;
			pos *= speed_mul * notespeed;
			switch ((*bmsnote)[channel][currentnotebar].type) {
			case BmsNote::NOTE_NORMAL:
				playobj->RenderNote(lane, pos);
				break;
			case BmsNote::NOTE_LNSTART:
				lnpos[channel] = pos;
				lnstart[channel] = true;
				break;
			case BmsNote::NOTE_LNEND:
				if (!lnstart[channel]) {
					playobj->RenderNote(lane, 0, pos);
				}
				playobj->RenderNote(lane, lnpos[channel], pos);
				lnstart[channel] = false;
				break;
			}
			if (pos > 1) break;
			currentnotebar = GetAvailableNoteIndex(channel, currentnotebar+1);
		}
		// draw last ln
		if (lnstart[channel]) {
			playobj->RenderNote(lane, lnpos[channel], 2.0);
		}
	}
}

// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto(PlayerPlayConfig *config, BmsNoteContainer *note, int playside, int playmode)
	: Player(config, note, playside, playmode, PLAYERTYPE::AUTO) {}

void PlayerAuto::Update() {
	// get time
	currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	currentbar = BmsHelper::GetCurrentBar();

	// check for note judgement
	for (int i = 0; i < 20; i++) {
		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (IsNoteAvailable(i) && noteindex[i] <= currentbar) {
			// if note is mine, then lets ignore as fast as we can
			if (GetCurrentNote(i)->type == BmsNote::NOTE_MINE) {
				// ignore the note
			}
			else {
				// Autoplay -> Automatically play
				BmsNote& note = (*bmsnote)[i][noteindex[i]];
				if (GetCurrentNote(i)->type == BmsNote::NOTE_LNSTART) {
					PLAYSOUND(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i, true);
					// we won't judge on LNSTART
					longnotestartpos[i] = noteindex[i];
					if (pLanehold[i]) pLanehold[i]->Start();
					if (pLanepress[i]) {
						pLaneup[i]->Stop();
						pLanepress[i]->Start();
					}
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					longnotestartpos[i] = -1;
					if (pLanehold[i]) pLanehold[i]->Stop();
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_NORMAL) {
					PLAYSOUND(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					if (pLanepress[i]) {
						pLaneup[i]->Stop();
						pLanepress[i]->Start();
					}
				}
			}

			// fetch next note
			noteindex[i] = GetNextAvailableNoteIndex(i);
		}

		/*
		 * pressing lane is automatically up-ped
		 * about after 50ms.
		 */
		if (pLanehold[i] && pLanehold[i]->IsStarted())
			pLanepress[i]->Start();
		if (pLanepress[i] && pLaneup[i] && pLaneup[i]->Trigger(pLanepress[i]->GetTick() > 50)) {
			pLanepress[i]->Stop();
		}
	}
}

void PlayerAuto::PressKey(int channel) {
	// do nothing
}

void PlayerAuto::UpKey(int channel) {
	// do nothing
}

void PlayerAuto::SetGoal(double rate) {
	targetrate = rate;
}

// ------ PlayerGhost ----------------------------

PlayerGhost::PlayerGhost(PlayerPlayConfig *config, BmsNoteContainer *note, int playside, int playmode)
	: Player(config, note, playside, playmode, PLAYERTYPE::AUTO) {}