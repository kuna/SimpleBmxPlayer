#include "player.h"
#include "global.h"
#include "handlerargs.h"
#include "bmsresource.h"
#include "ActorRenderer.h"
#include "util.h"
#include <time.h>

// global
Player*					PLAYER[4];			// player object
PlayerRenderValue		PLAYERVALUE[4];		// player variables for play

// macro
#define PLAYSOUND(k)	BmsResource::SOUND.Play(k)

// ------- Player ----------------------------------------

Player::Player(int playside, int playmode, int playertype) {
	this->playconfig = &PLAYERINFO[playside].playconfig;
	this->playside = playside;
	this->playmode = playmode;
	this->playertype = playertype;
	this->issilent = false;

	// initalize basic variables/timers
	pv = &PLAYERVALUE[playside];
	// DP is only allowed for playside 0
	if (playmode >= 10 && playside == 0) pv_dp = &PLAYERVALUE[playside + 1];
	else pv_dp = 0;

	for (int i = 0; i < 10; i++) {
		pLanepress[i] = pv->pLanepress[i];
		pLaneup[i] = pv->pLaneup[i];
		pLanehold[i] = pv->pLanehold[i];
		pLanejudgeokay[i] = pv->pLanejudgeokay[i];
		if (pv_dp) {
			pLanepress[i + 10] = pv_dp->pLanepress[i];
			pLaneup[i + 10] = pv_dp->pLaneup[i];
			pLanehold[i + 10] = pv_dp->pLanehold[i];
			pLanejudgeokay[i + 10] = pv_dp->pLanejudgeokay[i];
		}
		else {
			pLanehold[i + 10] = 0;
		}
	}
	judgeoffset = playconfig->judgeoffset;
	judgecalibration = playconfig->judgecalibration;
	pBmstimer = TIMERPOOL->Get("OnGameStart");
	memset(ispress_, 0, sizeof(ispress_));
	memset(islongnote_, 0, sizeof(islongnote_));

	// initalize note iterator (dummy)
	// also score / gauge.
	bmsnote = new BmsNoteManager();
	Reset(0);
	InitalizeGauge();
	InitalizeScore();

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
	// set sudden/lift first (floatspeed is relative to them)
	SetSudden(playconfig->sudden);
	SetLift(playconfig->lift);
	if (playconfig->usefloatspeed) {
		SetFloatSpeed(playconfig->floatspeed);
	}
	else {
		SetSpeed(playconfig->speed);
	}

	// TODO: set OP switch... no, in main?
}

Player::~Player() {
	// delete note object
	SAFE_DELETE(bmsnote);
}

void Player::InitalizeNote() {
	//
	// remove note data if previously existed
	//
	if (bmsnote)
		SAFE_DELETE(bmsnote);

	//
	// create note object
	//
	bmsnote = new BmsNoteManager();
	BmsResource::BMS.GetNoteData(*bmsnote);
	memset(&score, 0, sizeof(score));
	score.totalnote = bmsnote->GetNoteCount();

	//
	// reset TOTAL with note object
	// (IIDX style gauge TOTAL)
	//
	double total_iidx = bmsnote->GetTotalFromNoteCount();
	double total = BmsResource::BMS.GetTotal(total_iidx);
	int notecnt = score.totalnote;
	if (notecnt <= 0)
		notecnt = 1;			// div by 0
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

	//
	// before resetting iterator, modify note data
	//
	int seed = GamePlay::P.rseed;		// use gameplay's seed
	switch (PLAYERINFO[playside].playconfig.op_1p) {
	case OPTYPE::NONE:
		break;
	case OPTYPE::RANDOM:
		bmsnote->Random(seed);
		break;
	case OPTYPE::RRANDOM:
		bmsnote->RRandom(seed);
		break;
	case OPTYPE::SRANDOM:
		bmsnote->SRandom(seed);
		break;
	case OPTYPE::HRANDOM:
		// TODO: assist option should being.
		bmsnote->HRandom(seed);
		break;
	case OPTYPE::MIRROR:
		bmsnote->Mirror(seed);
		break;
	}

	//
	// reset iterator
	//
	Reset(0);
}

/*
 * hmm little amgibuous ...
 * - coursemode: don't initalize gauge.
 */
void Player::InitalizeGauge() {
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
}

/*
 * MUST reset for every game
 */
void Player::InitalizeScore() {
	// clear score
	score.Clear();
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

void Player::Silent(bool b) {
	issilent = b;
}

void Player::PlaySound(BmsWord& value) {
	if (!pBmstimer->IsStarted()) return;
	if (!issilent) PLAYSOUND(value);
}



/*------------------------------------------------------------------*
 * Speed related (TODO: floating speed)
 *------------------------------------------------------------------*/

namespace {
	double ConvertSpeedToFloat(double speed, double bpm) {
		/*
		 * the time 1 beat needs to go off:
		 * 1 / (speed) / (BPM) * 60 (sec)
		 */
		return 1.0 / speed / bpm * 60 * 2;
	}

	double ConvertFloatToSpeed(double speed, double bpm) {
		/*
		 * just reverse equation
		 * 1 / (float) / (BPM) * 60 (speed) 
		 */
		return 1.0 / speed / bpm * 60 * 2;
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
	*pv->pNoteSpeed = notespeed * 100 + 0.5;
	*pv->pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = 1000 * suddenheight;
	*pv->pLift = 1000 * liftheight;
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
	*pv->pNoteSpeed = notespeed * 100 + 0.5;
	*pv->pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = 1000 * suddenheight;
	*pv->pLift = 1000 * liftheight;
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(notefloat + speed); }

void Player::SetSudden(double height) {
	playconfig->sudden
		= suddenheight
		= height;

	/*
	 * set the pointer values
	 */
	*pv->pSudden_d = height;
	*pv->pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = 1000 * suddenheight;
	*pv->pLift = 1000 * liftheight;
}
void Player::DeltaSudden(double height) { SetSudden(suddenheight + height); }

void Player::SetLift(double height) {
	playconfig->lift
		= liftheight
		= height;

	/*
	 * set the pointer values
	 */
	*pv->pLift_d = height;
	*pv->pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	*pv->pSudden = 1000 * suddenheight;
	*pv->pLift = 1000 * liftheight;
}
void Player::DeltaLift(double height) { SetLift(liftheight + height); }
double Player::GetSpeedMul() { return notespeed * speed_mul; }	// for rendering





/*------------------------------------------------------------------*
* Judge related
*------------------------------------------------------------------*/

int Player::CheckJudgeByTiming(int delta) {
	int abs_delta = abs(delta);
	if (abs_delta <= BmsJudgeTiming::PGREAT)
		return JUDGETYPE::JUDGE_PGREAT;
	else if (abs_delta <= BmsJudgeTiming::GREAT)
		return JUDGETYPE::JUDGE_GREAT;
	else if (abs_delta <= BmsJudgeTiming::GOOD)
		return JUDGETYPE::JUDGE_GOOD;
	else if (abs_delta <= BmsJudgeTiming::BAD)
		return JUDGETYPE::JUDGE_BAD;
	else if (delta > 0 && abs_delta <= BmsJudgeTiming::NPOOR)
		// ÍöPOOR is ONLY allowed in case of early pressing
		return JUDGETYPE::JUDGE_NPOOR;
	else if (delta < 0)
		// if later then BAD timing, then it's POOR
		//return JUDGETYPE::JUDGE_LATE;
		return JUDGETYPE::JUDGE_POOR;
	else
		// eariler then ÍöPOOR -> no judge
		return JUDGETYPE::JUDGE_EARLY;
}

void Player::MakeJudge(int judgetype, int time, int channel, int fastslow, bool silent) {
	// if judgetype is not valid (TOO LATE or TOO EARLY) then ignore
	if (judgetype >= 10) return;
	// get current judgeside
	int judgeside = 0;
	if (playmode >= 10) {
		// DP
		if (channel < 10) judgeside = 1;
		else judgeside = 2;
	}
	else {
		// SP
		judgeside = 1;
	}

	/*
	 * score part
	 */
	// add current judge to grade
	score.AddGrade(judgetype);
	// current pgreat
	*pv->pNotePerfect = score.score[5];
	*pv->pNoteGreat = score.score[4];
	*pv->pNoteGood = score.score[3];
	*pv->pNoteBad = score.score[2];
	*pv->pNotePoor = score.score[1] + score.score[0];

	//
	// if currently slient mode, then don't process timers / values.
	//
	if (issilent) return;

	/*
	 * timer part
	 */
	const int channel_p = channel % 10;
	switch (judgeside) {
	case 1:
		if (judgetype >= JUDGETYPE::JUDGE_GREAT)
			pv->pLanejudgeokay[channel_p]->Start();
		// update graph/number
		if (judgetype <= JUDGETYPE::JUDGE_BAD)
			pv->pOnMiss->Start();
		// slow/fast?
		if (judgetype < JUDGETYPE::JUDGE_PGREAT) {
			if (fastslow == 1) {
				pv->pOnFast->Start();
				pv->pOnSlow->Stop();
			}
			else if (fastslow == 2) {
				pv->pOnFast->Stop();
				pv->pOnSlow->Start();
			}
		}
		else {
			pv->pOnFast->Stop();
			pv->pOnSlow->Stop();
		}
		break;
	case 2:
		if (judgetype >= JUDGETYPE::JUDGE_GREAT)
			pv_dp->pLanejudgeokay[channel_p]->Start();
		// update graph/number
		if (judgetype <= JUDGETYPE::JUDGE_BAD)
			pv_dp->pOnMiss->Start();
		// slow/fast
		if (judgetype < JUDGETYPE::JUDGE_PGREAT) {
			if (fastslow == 1) {
				pv_dp->pOnFast->Start();
				pv_dp->pOnSlow->Stop();
			}
			else if (fastslow == 2) {
				pv_dp->pOnFast->Stop();
				pv_dp->pOnSlow->Start();
			}
		}
		else {
			pv->pOnFast->Stop();
			pv->pOnSlow->Stop();
		}
		break;
	}
	// auto judge offset / add fast-slow grade
	if (judgetype < JUDGETYPE::JUDGE_PGREAT) {
		if (fastslow == 1) {
			score.fast++;
			if (judgecalibration) judgeoffset++;
		}
		else if (fastslow == 2) {
			score.slow++;
			if (judgecalibration) judgeoffset--;
		}
	}
	// reached rank?
	const int scoregrade = score.CalculateGrade();
	pv->pOnReachAAA->Trigger(scoregrade >= GRADETYPE::GRADE_AAA);
	pv->pOnReachAA->Trigger(scoregrade >= GRADETYPE::GRADE_AA);
	pv->pOnReachA->Trigger(scoregrade >= GRADETYPE::GRADE_A);
	pv->pOnReachB->Trigger(scoregrade >= GRADETYPE::GRADE_B);
	pv->pOnReachC->Trigger(scoregrade >= GRADETYPE::GRADE_C);
	pv->pOnReachD->Trigger(scoregrade >= GRADETYPE::GRADE_D);
	pv->pOnReachE->Trigger(scoregrade >= GRADETYPE::GRADE_E);
	pv->pOnReachF->Trigger(scoregrade >= GRADETYPE::GRADE_F);
	pv->pOnAAA->Trigger(scoregrade == GRADETYPE::GRADE_AAA);
	pv->pOnAA->Trigger(scoregrade == GRADETYPE::GRADE_AA);
	pv->pOnA->Trigger(scoregrade == GRADETYPE::GRADE_A);
	pv->pOnB->Trigger(scoregrade == GRADETYPE::GRADE_B);
	pv->pOnC->Trigger(scoregrade == GRADETYPE::GRADE_C);
	pv->pOnD->Trigger(scoregrade == GRADETYPE::GRADE_D);
	pv->pOnE->Trigger(scoregrade == GRADETYPE::GRADE_E);
	pv->pOnF->Trigger(scoregrade == GRADETYPE::GRADE_F);
	pv->pOnAAA->OffTrigger(scoregrade != GRADETYPE::GRADE_AAA);
	pv->pOnAA->OffTrigger(scoregrade != GRADETYPE::GRADE_AA);
	pv->pOnA->OffTrigger(scoregrade != GRADETYPE::GRADE_A);
	pv->pOnB->OffTrigger(scoregrade != GRADETYPE::GRADE_B);
	pv->pOnC->OffTrigger(scoregrade != GRADETYPE::GRADE_C);
	pv->pOnD->OffTrigger(scoregrade != GRADETYPE::GRADE_D);
	pv->pOnE->OffTrigger(scoregrade != GRADETYPE::GRADE_E);
	pv->pOnF->OffTrigger(scoregrade != GRADETYPE::GRADE_F);
	// TODO pacemaker
	// update gauge
	SetGauge(playergauge + notehealth[judgetype]);
	// update exscore/combo/etc ...
	const double rate_ = score.CalculateRate();
	const double rate_cur_ = score.CurrentRate();
	*pv->pExscore_d = rate_;		// graph
	*pv->pHighscore_d = rate_;		// graph
	*pv->pScore = score.CalculateScore();
	*pv->pExscore = score.CalculateEXScore();
	*pv->pRate_d = rate_cur_;
	*pv->pRate = rate_cur_ * 100;
	*pv->pMaxCombo = score.maxcombo;
	// fullcombo/lastnote check
	pv->pOnfullcombo->Trigger(score.combo == score.totalnote);
	pv->pOnlastnote->Trigger(score.LastNoteFinished());

	// record into playrecord
	replay_cur.AddJudge(time, judgeside, judgetype, fastslow, silent);
	
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

void Player::Save() {
	// TODO
	RString current_hash = GamePlay::P.bmshash[GamePlay::P.round - 1];
	RString player_name = PLAYERINFO[0].name;

	// create new record with current playdata
	PlayerSongRecord record;
	record.hash = current_hash;
	if (!PlayerRecordHelper::LoadPlayerRecord(record, PLAYERINFO[0].name, current_hash)) {
		record.clearcount = 0;
		record.failcount = 0;
	}
	if (IsDead() || (!dieonnohealth && playergauge < 0.8))
		record.failcount++;
	else
		record.clearcount++;
	record.maxcombo = score.maxcombo;
	record.minbp = score.score[0] + score.score[1] + score.score[2];
	record.score = score;

	// save
	PlayerRecordHelper::SavePlayerRecord(record, player_name);
	PlayerReplayHelper::SaveReplay(
		replay_cur,
		player_name,
		current_hash
	);
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
		iter_begin_[i] = (*bmsnote)[i].Begin();
		iter_end_[i] = (*bmsnote)[i].End();
	}
}

bool Player::IsNoteAvailable(int lane) {
	return iter_judge_[lane] != iter_end_[lane];
}

void Player::NextNote(int lane) {
	// search for next judged note
	if (IsNoteAvailable(lane)) ++iter_judge_[lane];
}

void Player::UpdateBasic() {
	// basic timer/value update
	pv->pOnCombo->OffTrigger(pv->pOnCombo->GetTick() > 500);
	pv->pOnMiss->OffTrigger(pv->pOnMiss->GetTick() > 1000);
	pv->pOnFast->OffTrigger(pv->pOnFast->GetTick() > 500);
	pv->pOnSlow->OffTrigger(pv->pOnSlow->GetTick() > 500);
	if (pv_dp) {
		pv_dp->pOnCombo->OffTrigger(pv_dp->pOnCombo->GetTick() > 500);
		pv_dp->pOnMiss->OffTrigger(pv_dp->pOnMiss->GetTick() > 1000);
		pv_dp->pOnFast->OffTrigger(pv_dp->pOnFast->GetTick() > 500);
		pv_dp->pOnSlow->OffTrigger(pv_dp->pOnSlow->GetTick() > 500);
	}
}

void Player::Update() {
	UpdateBasic();

	// if gameover then don't update
	if (IsDead()) return;

	// get time
	Uint32 currenttime = pBmstimer->GetTick() + judgeoffset;

	// check for note judgement
	// COMMENT: `int i` means lane index, NOT channel number.
	for (int i = 0; i < 20; i++) {
		/*
		 * If (next note exists)
		 */
		while (IsNoteAvailable(i)) {
			// skip hidden note
			BmsNote& note = iter_judge_[i]->second;
			if ((note.type == BmsNote::NOTE_HIDDEN && 
				note.time - BmsJudgeTiming::POOR < currenttime) ||
				note.type == BmsNote::NOTE_NONE)
				NextNote(i);
			else break;
		}
		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (
			IsNoteAvailable(i) && 
			iter_judge_[i]->second.time <= currenttime
			) {
			BmsNote& note = iter_judge_[i]->second;
			// if note is mine, then lets ignore as fast as we can
			if (note.type == BmsNote::NOTE_MINE) {
				NextNote(i);
			}
			// if note is press(HELL CHARGE), judge from pressing status
			else if (note.type == BmsNote::NOTE_PRESS) {
				MakeJudge(ispress_[i] ? JUDGETYPE::JUDGE_PGREAT : JUDGETYPE::JUDGE_POOR,
					currenttime, i);
				NextNote(i);
			}
			// if not autoplay, check timing for poor
			else if (CheckJudgeByTiming(note.time - currenttime + judgeoffset)
				== JUDGETYPE::JUDGE_POOR) {
				//
				// if late, POOR judgement is always occured
				// but we need to take care of in case of LongNote
				//
				// if LNSTART late, get POOR for LNEND (2 miss)
				if (note.type == BmsNote::NOTE_LNSTART) {
					MakeJudge(JUDGETYPE::JUDGE_POOR, currenttime, i);	// LNEND's poor
				}
				// if LNEND late, reset longnote pressing
				else if (note.type == BmsNote::NOTE_LNEND && islongnote_[i]) {
					islongnote_[i] = false;
					pLanehold[i]->Stop();
				}
				// make POOR judge
				// (CLAIM) if hidden note isn't ignored by NextAvailableNote(), 
				//         you have to hit it or you'll get miss.
				MakeJudge(JUDGETYPE::JUDGE_POOR, currenttime, i);
				NextNote(i);
			}
			else {
				// Don't skip note; it's not timing yet. wait next turn.
				break;
			}
		}
	}
}

// key input
void Player::UpKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// if not DP then make lane single
	if (playmode < 10) lane = lane % 10;

	// record to replay
	Uint32 currenttime = pBmstimer->GetTick();
	replay_cur.AddPress(currenttime, lane, 0);

	// if scratch, then set timer
	// (TODO)

	// convert SCDOWN to SCUP
	// (comment: in popn music, we don't do it.)
	if (lane == PlayerKeyIndex::P1_BUTTONSCDOWN) lane = PlayerKeyIndex::P1_BUTTONSCUP;
	if (lane == PlayerKeyIndex::P2_BUTTONSCDOWN) lane = PlayerKeyIndex::P2_BUTTONSCUP;

	// generally upkey do nothing
	// but works only if you pressing longnote
	if (islongnote_[lane] && GetCurrentNote(lane)->type == BmsNote::NOTE_LNEND) {
		// get judge
		int delta = iter_judge_[lane]->second.time - currenttime + judgeoffset;
		int judge = CheckJudgeByTiming(delta);
		if (judge == JUDGETYPE::JUDGE_EARLY || judge == JUDGETYPE::JUDGE_NPOOR)
			judge = JUDGETYPE::JUDGE_POOR;
		MakeJudge(judge, currenttime, lane, delta > 0 ? 1 : 2);
		// you're not longnote anymore~
		pLanehold[lane]->Stop();
		islongnote_[lane] = false;
		// focus judging note to next one
		NextNote(lane);
	}


	// trigger time & set value
	pLanepress[lane]->Stop();
	pLaneup[lane]->Start();
	ispress_[lane] = false;
}

void Player::PressKey(int lane) {
	// if gameover then ignore
	if (IsDead()) return;
	// if already pressed then ignore
	if (ispress_[lane]) return;
	// if not DP then make lane single
	if (playmode < 10) lane = lane % 10;

	//
	// record to replay
	//
	Uint32 currenttime = pBmstimer->GetTick();
	replay_cur.AddPress(currenttime, lane, 1);

	// if scratch, then set timer
	// (TODO)

	// convert SCDOWN to SCUP (do it before judge)
	// (comment: in popn music, we don't do it.)
	if (lane == PlayerKeyIndex::P1_BUTTONSCDOWN) lane = PlayerKeyIndex::P1_BUTTONSCUP;
	if (lane == PlayerKeyIndex::P2_BUTTONSCDOWN) lane = PlayerKeyIndex::P2_BUTTONSCUP;

	/*
	 * judge process start
	 */

	// pressed!
	ispress_[lane] = true;
	pLanepress[lane]->Start();
	pLaneup[lane]->Stop();

	//
	// cache current judge index first
	// because current key index will be changed after making judgement
	//
	BmsNoteLane::Iterator iter_sound_ = iter_judge_[lane];

	//
	// make judge
	//
	if (IsNoteAvailable(lane)) {
		int delta = iter_judge_[lane]->second.time - currenttime + judgeoffset;
		int fastslow = delta > 0 ? 1 : 2;
		int judge = CheckJudgeByTiming(delta);
		// only continue judging if judge isn't too fast (no judge)
		if (judge != JUDGETYPE::JUDGE_EARLY) {
			// if ÍöPOOR,
			// then don't process note - just judge as NPOOR
			if (judge == JUDGETYPE::JUDGE_NPOOR) {
				MakeJudge(judge, currenttime, lane, fastslow);
			}
			else {
				if (GetCurrentNote(lane)->type == BmsNote::NOTE_LNSTART) {
					// set longnote status
					MakeJudge(judge, currenttime, lane, fastslow, true);
					pLanehold[lane]->Start();
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
					MakeJudge(judge, currenttime, lane, fastslow);
				}

				// only go for next note if judge is over poor
				// (don't allow Íöpoor)
				NextNote(lane);
			}
		}
	}

	//
	// play sound
	// - if current lane has no note, then don't play
	//
	if (iter_sound_ == iter_end_[lane]) {
		// if no more note exists(end of the game), find previous note to play
		if (iter_sound_ == iter_begin_[lane]) return;
		--iter_sound_;
	}
	PlaySound(iter_sound_->second.value);
}

bool Player::IsDead() {
	return pv->pOnGameover->IsStarted();
}






// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto(int playside, int playmode)
	: Player(playside, playmode, PLAYERTYPE::AUTO) {
	targetrate = 1;
}

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
	for (int i = 0; i < 20; i++) {
		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (IsNoteAvailable(i) && iter_judge_[i]->first <= currentbar) {
			BmsNote& note = iter_judge_[i]->second;

			/*
			 * should decide what judge is proper for next
			 */
			int newjudge = JUDGETYPE::JUDGE_GOOD;
			int newexscore = (score.GetJudgedNote() + 1) * targetrate * 2 + 0.5;
			int updateexscore = newexscore - score.CalculateEXScore();
			if (updateexscore == 1) newjudge = JUDGETYPE::JUDGE_GREAT;
			else if (updateexscore >= 2) newjudge = JUDGETYPE::JUDGE_PGREAT;

			// if note is mine, then lets ignore as fast as we can
			if (note.type == BmsNote::NOTE_MINE) {
			}
			// if note is press, just combo up
			else if (note.type == BmsNote::NOTE_PRESS) {
				MakeJudge(newjudge, currenttime, i);
			}
			else {
				// Autoplay -> Automatically play
				if (note.type == BmsNote::NOTE_LNSTART) {
					PlaySound(note.value);
					MakeJudge(newjudge, currenttime, i, true);
					// we won't judge on LNSTART
					islongnote_[i] = true;
					pLanehold[i]->Start();
					pLaneup[i]->Stop();
					pLanepress[i]->Start();
				}
				else if (note.type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(newjudge, currenttime, i);
					islongnote_[i] = false;
					pLanehold[i]->Stop();
				}
				else if (note.type == BmsNote::NOTE_NORMAL) {
					PlaySound(note.value);
					MakeJudge(newjudge, currenttime, i);
					pLaneup[i]->Stop();
					pLanepress[i]->Start();
				}
			}

			// fetch next note
			NextNote(i);
		}

		/*
		 * pressing lane is automatically up-ped
		 * about after 50ms.
		 */
		if (!pLanehold[i]) continue;
		if (pLanehold[i]->IsStarted())
			pLanepress[i]->Start();
		if (pLaneup[i]->Trigger(pLanepress[i]->GetTick() > 50))
			pLanepress[i]->Stop();
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

PlayerReplay::PlayerReplay(int playside, int playmode)
	: Player(playside, playmode, PLAYERTYPE::AUTO) {}

void PlayerReplay::SetReplay(const PlayerReplayRecord &rep) {
	replay = rep;
	iter_ = replay.Begin();
}

void PlayerReplay::Update() {
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

	// update judge & note press
	while (iter_ != replay.End() && iter_->time <= currenttime) {
		if (iter_->lane >= 0xA0) {
			// score up! (SP/DP)
			int silent = iter_->value / 256;
			int fastslow = iter_->value / 16;
			int s = iter_->value % 16;
			int playside = iter_->lane - 0xA0;
			MakeJudge(s, iter_->time, playside, fastslow, silent);
		}
		else {
			// just press / up lane
			// keysound, pressing effect should available here ...
			int lane = iter_->lane;
			if (iter_->value == 1) {
				pLaneup[lane]->Stop();
				pLanepress[lane]->Start();
			}
			else {
				pLaneup[lane]->Stop();
				pLanepress[lane]->Start();
			}
			switch (iter_judge_[lane]->second.type) {
			case BmsNote::NOTE_LNSTART:
				pLanehold[lane]->Start();
				break;
			case BmsNote::NOTE_LNEND:
				pLanehold[lane]->Stop();
				break;
			case BmsNote::NOTE_NORMAL:
				break;
			case BmsNote::NOTE_MINE:
				// ??
				break;
			}
		}
		++iter_;
	}

	// basic update; ignore transparent & mine note
	for (int i = 0; i < 20; i++) {
		int notetime = iter_judge_[i]->second.time;
		int notetype = iter_judge_[i]->second.type;
		while (
			notetime - BmsJudgeTiming::POOR / 1000.0 < currenttime && notetype == BmsNote::NOTE_HIDDEN ||
			notetime < currenttime && notetype == BmsNote::NOTE_MINE ||
			notetype == BmsNote::NOTE_NONE
			) {
			iter_judge_[i]++;
		}

		// if holdnote, then lanepress -> stay active!
		if (pLanehold[i]->IsStarted())
			pLanepress[i]->Start();
	}
}

void PlayerReplay::PressKey(int channel) {
	// do nothing
}

void PlayerReplay::UpKey(int channel) {
	// do nothing
}