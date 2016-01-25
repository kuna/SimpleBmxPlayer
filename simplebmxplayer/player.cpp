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

// ------- Player ----------------------------------------

Player::Player(PlayerPlayConfig* config, BmsNoteManager *note, 
	int playside, int playmode, int playertype) {
	this->playconfig = config;
	this->playside = playside;
	this->playmode = playmode;
	this->playertype = playertype;

	// initalize basic variables/timers
	pv = &PLAYERVALUE[playside];
	if (playmode >= 10) pv_dp = &PLAYERVALUE[playside + 1];
	else pv_dp = 0;
	pBmstimer = TIMERPOOL->Get("OnGameStart");

	// initalize note/score
	bmsnote = note;
	memset(&score, 0, sizeof(score));
	int notecnt
		= score.totalnote
		= bmsnote->GetNoteCount();
	judgenotecnt = 0;
	for (int i = 0; i < 20; i++) {
		noteindex[i] = GetAvailableNoteIndex(i);
	}

	// initialize gauge
	playergaugetype
		= *pv->pGaugeType
		= playconfig->gaugetype;
	switch (playergaugetype) {
	case GAUGETYPE::GROOVE:
	case GAUGETYPE::EASY:
	case GAUGETYPE::ASSISTEASY:
		SetGauge(0.2);
		dieonnohealth = false;
		break;
	default:
		SetGauge(1.0);
		dieonnohealth = true;
		break;
	}
	memset(notehealth, 0, sizeof(notehealth));
	double total = 400;		// TODO: calculate TOTAL from BmsHelper. // TODO: sum all if in grade mode.
	switch (playergaugetype) {
	case GAUGETYPE::GROOVE:
		notehealth[5] = total / notecnt / 100;
		notehealth[4] = total / notecnt / 100;
		notehealth[3] = total / notecnt / 2 / 100;
		notehealth[2] = -2.0 / 100;
		notehealth[1] = -6.0 / 100;
		notehealth[0] = -2.0 / 100;
		break;
	case GAUGETYPE::EASY:
	case GAUGETYPE::ASSISTEASY:
		notehealth[5] = total / notecnt / 100;
		notehealth[4] = total / notecnt / 100;
		notehealth[3] = total / notecnt / 2 / 100;
		notehealth[2] = -1.6 / 100;
		notehealth[1] = -4.8 / 100;
		notehealth[0] = -1.6 / 100;
		break;
	case GAUGETYPE::HARD:
		notehealth[5] = 0.16 / 100;
		notehealth[4] = 0.16 / 100;
		notehealth[3] = 0;
		notehealth[2] = -5.0 / 100;
		notehealth[1] = -9.0 / 100;
		notehealth[0] = -5.0 / 100;
		break;
	case GAUGETYPE::EXHARD:
		notehealth[5] = 0.16 / 100;
		notehealth[4] = 0.16 / 100;
		notehealth[3] = 0;
		notehealth[2] = -10.0 / 100;
		notehealth[1] = -18.0 / 100;
		notehealth[0] = -10.0 / 100;
		break;
	case GAUGETYPE::GRADE:
		notehealth[5] = 0.16 / 100;
		notehealth[4] = 0.16 / 100;
		notehealth[3] = 0.04 / 100;
		notehealth[2] = -1.5 / 100;
		notehealth[1] = -2.5 / 100;
		notehealth[0] = -1.5 / 100;
		break;
	case GAUGETYPE::EXGRADE:
		notehealth[5] = 0.16 / 100;
		notehealth[4] = 0.16 / 100;
		notehealth[3] = 0.04 / 100;
		notehealth[2] = -3.0 / 100;
		notehealth[1] = -5.0 / 100;
		notehealth[0] = -3.0 / 100;
		break;
	case GAUGETYPE::PATTACK:
		notehealth[5] = 0;
		notehealth[4] = 0;
		notehealth[3] = -1;
		notehealth[2] = -1;
		notehealth[1] = -1;
		notehealth[0] = -1;
		break;
	case GAUGETYPE::HAZARD:
		notehealth[5] = 0;
		notehealth[4] = 0;
		notehealth[3] = 0;
		notehealth[2] = -1;
		notehealth[1] = -1;
		notehealth[0] = 0;
		break;
	}

	// initialize play speed
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
	// TODO: set OP switch... no, in main?
}

void Player::SetGauge(double v) {
	if (v >= 1) {
		pv->pOnGaugeMax->Trigger();
		v = 1;
	}
	else {
		pv->pOnGaugeMax->Stop();
	}
	if (v < 0) {
		v = 0;
		pv->pOnGameover->Trigger(dieonnohealth);
	}
	*pv->pGauge_d = playergauge = v;
	int newgauge_int = (int)(v * 50) * 2;
	if (newgauge_int > *pv->pGauge)
		pv->pOnGaugeUp->Start();
	*pv->pGauge = newgauge_int;
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
	playconfig->speed
		= notespeed
		= speed;
	playconfig->floatspeed
		= notefloat
		= ConvertSpeedToFloat(speed, BmsHelper::GetCurrentBPM());

	/*
	 * set the pointer values
	 */
	*pv->pNoteSpeed = notespeed * 100;
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSuddenHeight = notefloat * suddenheight;
	*pv->pLiftHeight = notefloat * liftheight;
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
	*pv->pNoteSpeed = notespeed * 100;
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSuddenHeight = notefloat * suddenheight;
	*pv->pLiftHeight = notefloat * liftheight;
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(notefloat + speed); }

void Player::SetSudden(double height) {
	playconfig->sudden
		= suddenheight
		= height;

	/*
	 * set the pointer values
	 */
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSuddenHeight = notefloat * suddenheight;
	*pv->pLiftHeight = notefloat * liftheight;
}
void Player::DeltaSudden(double height) { SetSudden(suddenheight + height); }

void Player::SetLift(double height) {
	playconfig->lift
		= liftheight
		= height;

	/*
	 * set the pointer values
	 */
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSuddenHeight = notefloat * suddenheight;
	*pv->pLiftHeight = notefloat * liftheight;
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
	// get current judgeside
	int judgeside = 0;
	if (playmode >= 10) {
		// DP
		if (channel < 10) judgeside = 1;
		else judgeside = 2;
	}
	else {
		// SP
		if (playside == 0) judgeside = 1;
		else judgeside = 2;
	}
	int channel_p = channel % 10;

	switch (judgeside) {
	case 1:
		if (judgetype >= JUDGETYPE::JUDGE_GREAT)
			pv->pLanejudgeokay[channel_p]->Start();
		// update graph/number
		if (judgetype <= JUDGETYPE::JUDGE_BAD)
			pv->pOnMiss->Start();
		break;
	case 2:
		if (judgetype >= JUDGETYPE::JUDGE_GREAT)
			pv_dp->pLanejudgeokay[channel_p]->Start();
		// update graph/number
		if (judgetype <= JUDGETYPE::JUDGE_BAD)
			pv_dp->pOnMiss->Start();
		break;
	}

	score.AddGrade(judgetype);
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
	SetGauge(playergauge + notehealth[judgetype]);
	// update exscore/combo/etc ...
	*pv->pExscore_d = score.CalculateRate();
	*pv->pHighscore_d = score.CalculateRate();
	*pv->pScore = score.CalculateScore();
	*pv->pExscore = score.CalculateEXScore();
	*pv->pMaxCombo = score.maxcombo;
	// fullcombo/lastnote check
	if (score.combo == score.totalnote)
		pv->pOnfullcombo->Start();
	if (judgenotecnt == score.totalnote)
		pv->pOnlastnote->Start();

	if (!silent) {
		switch (judgeside) {
		case 1:
			// show judge(combo) element
			for (int i = 0; i < 6; i++) {
				if (i == judgetype)
					pv->pOnJudge[i]->Start();
				else
					pv->pOnJudge[i]->Stop();
			}
			pv->pOnCombo->Start();
			*pv->pCombo = score.combo;
			break;
		case 2:
			// show judge(combo) element
			for (int i = 0; i < 6; i++) {
				if (i == judgetype)
					pv_dp->pOnJudge[i]->Start();
				else
					pv_dp->pOnJudge[i]->Stop();
			}
			pv_dp->pOnCombo->Start();
			*pv_dp->pCombo = score.combo;
			break;
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
	pv->pOnCombo->OffTrigger(pv->pOnCombo->GetTick() > 500);
	pv->pOnMiss->OffTrigger(pv->pOnMiss->GetTick() > 1000);
	if (pv_dp) {
		pv_dp->pOnCombo->OffTrigger(pv_dp->pOnCombo->GetTick() > 500);
		pv_dp->pOnMiss->OffTrigger(pv_dp->pOnMiss->GetTick() > 1000);
	}

	// if gameover then ignore
	if (IsDead()) return;

	// get time
	currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	currentbar = BmsHelper::GetCurrentBar();

	// check for note judgement
	// COMMENT: `int i` means lane index, NOT channel number.
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
void Player::UpKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// make judge (if you're pressing longnote)
	if (IsLongNote(lane)) {
		if (IsNoteAvailable(lane) && GetCurrentNote(lane)->type == BmsNote::NOTE_LNEND) {
			double t = currenttime / 1000.0;
			// make judge
			int judge = CheckJudgeByTiming(t - BmsHelper::GetCurrentTimeFromBar(noteindex[lane]));
			MakeJudge(judge, lane);
			// get next note and remove current longnote
			(*bmsnote)[lane][longnotestartpos[lane]].type = BmsNote::NOTE_NONE;
			(*bmsnote)[lane][noteindex[lane]].type = BmsNote::NOTE_NONE;
			noteindex[lane] = GetNextAvailableNoteIndex(lane);
		}
		longnotestartpos[lane] = -1;
	}
}

void Player::PressKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// make judge
	if (IsNoteAvailable(lane)) {
		double t = currenttime / 1000.0;
		int judge = CheckJudgeByTiming(t - BmsHelper::GetCurrentTimeFromBar(noteindex[lane]));
		if (GetCurrentNote(lane)->type == BmsNote::NOTE_LNSTART) {
			// store longnote start pos & set longnote status
			MakeJudge(judge, lane, true);
			longnotestartpos[lane] = noteindex[lane];
		}
		else if (GetCurrentNote(lane)->type == BmsNote::NOTE_MINE) {
			// damage, ouch
			if (GetCurrentNote(lane)->value == BmsWord::MAX)
				health = 0;
			else
				health -= GetCurrentNote(lane)->value.ToInteger();
		}
		else if (GetCurrentNote(lane)->type == BmsNote::NOTE_NORMAL) {
			MakeJudge(judge, lane);
		}
	}
	PLAYSOUND(keysound[lane][currentbar].ToInteger());
}

double Player::GetLastMissTime() {
	// TODO
	return 0;
}

int Player::GetCurrentNoteBar(int channel) {
	return noteindex[channel];
}

bool Player::IsDead() {
	return pv->pOnGameover->IsStarted();
}

void Player::RenderNote(SkinPlayObject *playobj) {
	double currentpos = BmsHelper::GetCurrentPosFromTime(currenttime / 1000.0);

	// render judgeline/line
	int currentbar = BmsHelper::GetCurrentBar();
	for (; currentbar < BmsResource::BMSTIME.GetSize(); currentbar++) {
		if (BmsResource::BMSTIME[currentbar].measure) {
			double pos = BmsHelper::GetCurrentPosFromBar(currentbar) - currentpos;
			pos *= speed_mul * notespeed;
			if (pos > 1) break;
			playobj->RenderLine(pos);
		}
	}

	// in lane, 0 == SC
	// we have to check lane to ~ 20, actually.
	double lnpos[20] = { 0, };
	bool lnstart[20] = { false, };
	for (int lane = 0; lane < 20; lane++) {
		int currentnotebar = GetCurrentNoteBar(lane);
		std::vector<BmsNote>& lanearr = (*bmsnote)[lane];
		while (currentnotebar >= 0) {
			double pos = BmsHelper::GetCurrentPosFromBar(currentnotebar) - currentpos;
			pos *= speed_mul * notespeed;
			switch (lanearr[currentnotebar].type) {
			case BmsNote::NOTE_NORMAL:
				playobj->RenderNote(lane, pos);
				break;
			case BmsNote::NOTE_LNSTART:
				lnpos[lane] = pos;
				lnstart[lane] = true;
				break;
			case BmsNote::NOTE_LNEND:
				if (!lnstart[lane]) {
					playobj->RenderNote(lane, 0, pos);
				}
				playobj->RenderNote(lane, lnpos[lane], pos);
				lnstart[lane] = false;
				break;
			}
			if (pos > 1) break;
			currentnotebar = GetAvailableNoteIndex(lane, currentnotebar+1);
		}
		// draw last ln
		if (lnstart[lane]) {
			playobj->RenderNote(lane, lnpos[lane], 2.0);
		}
	}
}

// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto(PlayerPlayConfig *config, BmsNoteManager *note, int playside, int playmode)
	: Player(config, note, playside, playmode, PLAYERTYPE::AUTO) {}

void PlayerAuto::Update() {
	// timer/value update
	pv->pOnCombo->OffTrigger(pv->pOnCombo->GetTick() > 500);
	pv->pOnMiss->OffTrigger(pv->pOnMiss->GetTick() > 1000);
	if (pv_dp) {
		pv_dp->pOnCombo->OffTrigger(pv_dp->pOnCombo->GetTick() > 500);
		pv_dp->pOnMiss->OffTrigger(pv_dp->pOnMiss->GetTick() > 1000);
	}

	// get time
	currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	currentbar = BmsHelper::GetCurrentBar();

	// check for note judgement
	PlayerRenderValue *pv_ = pv;
	for (int i = 0; i < 20; i++) {
		/*
		 * watch out for DP
		 */
		if (i >= 10) pv_ = pv_dp;
		if (!pv_) continue;
		int ni = i % 10;

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
					pv_->pLanehold[ni]->Start();
					pv_->pLaneup[ni]->Stop();
					pv_->pLanepress[ni]->Start();
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					longnotestartpos[i] = -1;
					pv_->pLanehold[ni]->Stop();
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_NORMAL) {
					PLAYSOUND(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					pv_->pLaneup[ni]->Stop();
					pv_->pLanepress[ni]->Start();
				}
			}

			// fetch next note
			noteindex[i] = GetNextAvailableNoteIndex(i);
		}

		/*
		 * pressing lane is automatically up-ped
		 * about after 50ms.
		 */
		if (pv_->pLanehold[ni]->IsStarted())
			pv_->pLanepress[ni]->Start();
		if (pv_->pLaneup[ni]->Trigger(pv_->pLanepress[ni]->GetTick() > 50))
			pv_->pLanepress[ni]->Stop();
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

PlayerGhost::PlayerGhost(PlayerPlayConfig *config, BmsNoteManager *note, int playside, int playmode)
	: Player(config, note, playside, playmode, PLAYERTYPE::AUTO) {}