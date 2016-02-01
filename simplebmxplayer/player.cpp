#include "player.h"
#include "global.h"
#include "handlerargs.h"
#include "bmsresource.h"
#include "ActorRenderer.h"
#include "util.h"
#include <time.h>

// global
Player*		PLAYER[4];

// macro
#define PLAYSOUND(k)	BmsResource::SOUND.Play(k)

// ------- Player ----------------------------------------

Player::Player(PlayerPlayConfig* config, 
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
	BmsResource::BMS.GetNoteData(*bmsnote);
	memset(&score, 0, sizeof(score));
	int notecnt
		= score.totalnote
		= bmsnote->GetNoteCount();
	judgenotecnt = 0;
	Reset(0);

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
	// IIDX style gauge TOTAL
	double total_iidx = bmsnote->GetTotalFromNoteCount();
	double total = BmsResource::BMS.GetTotal(total_iidx);
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
		speed_mul = 120.0 / BmsResource::BMS.GetTimeManager().GetMaxBPM();
		break;
	case SPEEDTYPE::MINBPM:
		speed_mul = 120.0 / BmsResource::BMS.GetTimeManager().GetMinBPM();
		break;
	case SPEEDTYPE::MEDIUM:
		speed_mul = 120.0 / BmsResource::BMS.GetTimeManager().GetMediumBPM();
		break;
	}
	if (playconfig->usefloatspeed) {
		SetFloatSpeed(playconfig->floatspeed);
	}
	else {
		SetSpeed(playconfig->speed);
	}
	SetSudden(playconfig->sudden);
	SetLift(playconfig->lift);

	// TODO: set OP switch... no, in main?
}

Player::~Player() {
	// delete note object
	SAFE_DELETE(bmsnote);
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




/*------------------------------------------------------------------*
 * Speed related
 *------------------------------------------------------------------*/

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
	*pv->pSudden = notefloat * suddenheight;
	*pv->pLift = notefloat * liftheight;
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
	*pv->pSudden = notefloat * suddenheight;
	*pv->pLift = notefloat * liftheight;
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(notefloat + speed); }

void Player::SetSudden(double height) {
	playconfig->sudden
		= suddenheight
		= height;
	*pv->pSudden_d = height;

	/*
	 * set the pointer values
	 */
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = notefloat * suddenheight;
	*pv->pLift = notefloat * liftheight;
}
void Player::DeltaSudden(double height) { SetSudden(suddenheight + height); }

void Player::SetLift(double height) {
	playconfig->lift
		= liftheight
		= height;
	*pv->pLift_d = height;

	/*
	 * set the pointer values
	 */
	*pv->pFloatSpeed = notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = notefloat * suddenheight;
	*pv->pLift = notefloat * liftheight;
}
void Player::DeltaLift(double height) { SetLift(liftheight + height); }






/*------------------------------------------------------------------*
* Judge related
*------------------------------------------------------------------*/

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






/*------------------------------------------------------------------*
 * Game flow related
 *------------------------------------------------------------------*/

namespace {
	double GetTimeFromBar(barindex bar) {
		return BmsResource::BMS.GetTimeManager().GetTimeFromBar(bar);
	}
}

void Player::Reset(barindex bar) {
	//
	// use this function ONLY for resetting player to play again
	//

	// currently pressing longnote? reset.
	memset(islongnote_, 0, sizeof(islongnote_));
	// reset each of the note index
	for (int i = 0; i < 20; i++) {
		iter_judge_[i] = (*bmsnote)[i].Begin(bar);
		iter_end_[i] = (*bmsnote)[i].End();
	}
}

bool Player::IsNoteAvailable(int lane) {
	return iter_judge_[lane] != iter_end_[lane];
}

void Player::NextAvailableNote(int lane) {
	do {
		++iter_judge_[lane];
	} while (iter_judge_[lane]->second.type == BmsNote::NOTE_HIDDEN ||
		iter_judge_[lane]->second.type == BmsNote::NOTE_NONE);
}

void Player::Update() {
	// timer/value update
	pv->pOnCombo->OffTrigger(pv->pOnCombo->GetTick() > 500);
	pv->pOnMiss->OffTrigger(pv->pOnMiss->GetTick() > 1000);
	if (pv_dp) {
		pv_dp->pOnCombo->OffTrigger(pv_dp->pOnCombo->GetTick() > 500);
		pv_dp->pOnMiss->OffTrigger(pv_dp->pOnMiss->GetTick() > 1000);
	}

	// if gameover then don't update
	if (IsDead()) return;

	// get time
	Uint32 currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	barindex currentbar = BmsHelper::GetCurrentBar();

	// check for note judgement
	// COMMENT: `int i` means lane index, NOT channel number.
	for (int i = 0; i < 20; i++) {
		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (IsNoteAvailable(i) && iter_judge_[i]->first <= currentbar) {
			BmsNote& note = iter_judge_[i]->second;
			// if note is mine, then lets ignore as fast as we can
			if (note.type == BmsNote::NOTE_MINE) {
			}
			// if note is press(HELL CHARGE), judge from pressing status
			else if (note.type == BmsNote::NOTE_PRESS) {
				MakeJudge(ispress_[i] ? JUDGETYPE::JUDGE_PGREAT : JUDGETYPE::JUDGE_POOR, i);
			}
			// if not autoplay, check timing for poor
			else if (CheckJudgeByTiming(GetTimeFromBar(iter_judge_[i]->first) - time_sec)
				== JUDGETYPE::JUDGE_LATE) {
				//
				// if late, POOR judgement is always occured
				// but we need to take care of in case of LongNote
				//
				// if LNSTART late, get POOR for LNEND (2 miss)
				if (note.type == BmsNote::NOTE_LNSTART) {
					MakeJudge(JUDGETYPE::JUDGE_POOR, i);	// LNEND's poor
				}
				// if LNEND late, reset longnote pressing
				else if (note.type == BmsNote::NOTE_LNEND && islongnote_[i]) {
					islongnote_[i] = false;
				}
				// make POOR judge
				// (CLAIM) if hidden note isn't ignored by NextAvailableNote(), 
				// you have to hit it or you'll get miss.
				MakeJudge(JUDGETYPE::JUDGE_POOR, i);
			}

			// retrieve next note
			NextAvailableNote(i);
		}
	}
}

// key input
void Player::UpKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// generally upkey do nothing
	// but works only if you pressing longnote
	if (islongnote_[lane]) {
		if (IsNoteAvailable(lane) && GetCurrentNote(lane)->type == BmsNote::NOTE_LNEND) {
			double t = pBmstimer->GetTick() / 1000.0;
			int judge = CheckJudgeByTiming(t - GetTimeFromBar(iter_judge_[lane]->first));
			MakeJudge(judge, lane);
			// get next note and remove current longnote
			NextAvailableNote(lane);
		}
		islongnote_[lane] = false;
	}
}

void Player::PressKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// make judge
	if (IsNoteAvailable(lane)) {
		double t = pBmstimer->GetTick() / 1000.0;
		int judge = CheckJudgeByTiming(t - GetTimeFromBar(iter_judge_[lane]->first));
		// only continue judging if judge isn't too fast or too late
		if (judge != JUDGETYPE::JUDGE_LATE && judge != JUDGETYPE::JUDGE_EARLY) {
			if (GetCurrentNote(lane)->type == BmsNote::NOTE_LNSTART) {
				// set longnote status
				MakeJudge(judge, lane, true);
				islongnote_[lane] = true;
			}
			else if (GetCurrentNote(lane)->type == BmsNote::NOTE_MINE) {
				// mine damage
				if (GetCurrentNote(lane)->value == BmsWord::MAX)
					health = 0;
				else
					health -= GetCurrentNote(lane)->value.ToInteger();
			}
			else if (GetCurrentNote(lane)->type == BmsNote::NOTE_NORMAL) {
				MakeJudge(judge, lane);
			}
		}
	}
	//
	// now scan for current play sound
	// if current lane has no note, then don't play
	//
	BmsNoteLane::Iterator iter_sound_ = iter_judge_[lane];
	while ((iter_sound_->second.type == BmsNote::NOTE_HIDDEN)
		&& iter_sound_ != (*bmsnote)[lane].Begin()) --iter_sound_;
	PLAYSOUND(iter_sound_->second.value.ToInteger());
}

bool Player::IsDead() {
	return pv->pOnGameover->IsStarted();
}






// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto(PlayerPlayConfig *config, int playside, int playmode)
	: Player(config, playside, playmode, PLAYERTYPE::AUTO) {}

void PlayerAuto::Update() {
	// timer/value update
	pv->pOnCombo->OffTrigger(pv->pOnCombo->GetTick() > 500);
	pv->pOnMiss->OffTrigger(pv->pOnMiss->GetTick() > 1000);
	if (pv_dp) {
		pv_dp->pOnCombo->OffTrigger(pv_dp->pOnCombo->GetTick() > 500);
		pv_dp->pOnMiss->OffTrigger(pv_dp->pOnMiss->GetTick() > 1000);
	}

	// get time
	Uint32 currenttime = pBmstimer->GetTick();
	double time_sec = currenttime / 1000.0;

	// and recalculate note index/bgm index/position
	barindex currentbar = BmsHelper::GetCurrentBar();

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
		while (IsNoteAvailable(i) && iter_judge_[i]->first <= currentbar) {
			BmsNote& note = iter_judge_[i]->second;

			// if note is mine, then lets ignore as fast as we can
			if (note.type == BmsNote::NOTE_MINE) {
			}
			// if note is press, just combo up
			else if (note.type == BmsNote::NOTE_PRESS) {
				MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
			}
			else {
				// Autoplay -> Automatically play
				if (note.type == BmsNote::NOTE_LNSTART) {
					PLAYSOUND(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i, true);
					// we won't judge on LNSTART
					islongnote_[i] = true;
					pv_->pLanehold[ni]->Start();
					pv_->pLaneup[ni]->Stop();
					pv_->pLanepress[ni]->Start();
				}
				else if (note.type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					islongnote_[i] = false;
					pv_->pLanehold[ni]->Stop();
				}
				else if (note.type == BmsNote::NOTE_NORMAL) {
					PLAYSOUND(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					pv_->pLaneup[ni]->Stop();
					pv_->pLanepress[ni]->Start();
				}
			}

			// fetch next note
			NextAvailableNote(i);
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

PlayerGhost::PlayerGhost(PlayerPlayConfig *config, int playside, int playmode)
	: Player(config, playside, playmode, PLAYERTYPE::AUTO) {}