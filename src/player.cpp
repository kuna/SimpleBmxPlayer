#include "game.h"
#include "player.h"
#include "global.h"
#include "SongPlayer.h"
#include "Pool.h"
#include "util.h"
#include <time.h>

// global
Player*					PLAYER[4];			// player object

// macro
#define PLAYSOUND(k)	SONGPLAYER->PlayKeySound(k)

// ------- Player ----------------------------------------

Player::Player(int playside, int playertype) {
	//
	// basic player object setting
	// 
	this->playconfig = &PLAYERINFO[playside].playconfig;
	this->playertype = playertype;
	this->m_PlaySound = true;

	judgeoffset = playconfig->judgeoffset;
	judgecalibration = playconfig->judgecalibration;

	//
	// set metrics
	//

	pNoteSpeed.SetFromPool(playside, "Speed");
	pFloatSpeed.SetFromPool(playside, "FloatSpeed");
	pSudden.SetFromPool(playside, "Sudden");
	pLift.SetFromPool(playside, "Lift");
	pSudden_d.SetFromPool(playside, "Sudden");
	pLift_d.SetFromPool(playside, "Lift");

	pGauge_d.SetFromPool(playside, "Gauge");
	pGaugeType.SetFromPool(playside, "GaugeType");
	pGauge.SetFromPool(playside, "Gauge");
	pExscore.SetFromPool(playside, "ExScore");
	pScore.SetFromPool(playside, "Score");
	pExscore_d.SetFromPool(playside, "ExScore");
	pHighscore_d.SetFromPool(playside, "HighScore");
	pScore.SetFromPool(playside, "Score");
	pCombo.SetFromPool(playside, "Combo");
	pMaxCombo.SetFromPool(playside, "MaxCombo");
	pTotalnotes.SetFromPool(playside, "TotalNotes");
	pRivaldiff.SetFromPool(playside, "RivalDiff");
	pRate.SetFromPool(playside, "Rate");
	pTotalRate.SetFromPool(playside, "TotalRate");
	pRate_d.SetFromPool(playside, "Rate");
	pTotalRate_d.SetFromPool(playside, "TotalRate");

	pOnJudge[5].SetFromPool(playside, "JudgePerfect");
	pOnJudge[4].SetFromPool(playside, "JudgeGreat");
	pOnJudge[3].SetFromPool(playside, "JudgeGood");
	pOnJudge[2].SetFromPool(playside, "JudgeBad");
	pOnJudge[1].SetFromPool(playside, "JudgePoor");
	pOnJudge[0].SetFromPool(playside, "JudgePoor");
	pNotePerfect.SetFromPool(playside, "PerfectCount");
	pNoteGreat.SetFromPool(playside, "GreatCount");
	pNoteGood.SetFromPool(playside, "GoodCount");
	pNoteBad.SetFromPool(playside, "BadCount");
	pNotePoor.SetFromPool(playside, "PoorCount");
	pOnSlow.SetFromPool(playside, "Slow");
	pOnFast.SetFromPool(playside, "Fast");

	pOnAAA.SetFromPool(playside, "IsP1AAA");
	pOnAA.SetFromPool(playside, "IsP1AA");
	pOnA.SetFromPool(playside, "IsP1A");
	pOnB.SetFromPool(playside, "IsP1B");
	pOnC.SetFromPool(playside, "IsP1C");
	pOnD.SetFromPool(playside, "IsP1D");
	pOnE.SetFromPool(playside, "IsP1E");
	pOnF.SetFromPool(playside, "IsP1F");
	pOnReachAAA.SetFromPool(playside, "IsP1ReachAAA");
	pOnReachAA.SetFromPool(playside, "IsP1ReachAA");
	pOnReachA.SetFromPool(playside, "IsP1ReachA");
	pOnReachB.SetFromPool(playside, "IsP1ReachB");
	pOnReachC.SetFromPool(playside, "IsP1ReachC");
	pOnReachD.SetFromPool(playside, "IsP1ReachD");
	pOnReachE.SetFromPool(playside, "IsP1ReachE");
	pOnReachF.SetFromPool(playside, "IsP1ReachF");

	pOnMiss.SetFromPool(playside, "Miss");
	pOnCombo.SetFromPool(playside, "Combo");
	pOnfullcombo.SetFromPool(playside, "FullCombo");
	pOnlastnote.SetFromPool(playside, "LastNote");
	pOnGameover.SetFromPool(playside, "GameOver");
	pOnGaugeMax.SetFromPool(playside, "GaugeMax");
	pOnGaugeUp.SetFromPool(playside, "GaugeUp");

	/*
	* SC : note-index 0
	*/
	for (int i = 0; i < 20; i++) {
		m_Lane[i].pLanePress.SetFromPool(playside, ssprintf("Key%dPress", i));
		m_Lane[i].pLaneUp.SetFromPool(playside, ssprintf("Key%dUp", i));
		m_Lane[i].pLaneHold.SetFromPool(playside, ssprintf("Judge%dHold", i));
		m_Lane[i].pLaneOkay.SetFromPool(playside, ssprintf("Judge%dOkay", i));
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
	SWITCH_ON("IsBGA");
	SWITCH_ON("IsExtraMode");
	DOUBLEPOOL->Set("TargetExScore", 0.5);
	DOUBLEPOOL->Set("TargetExScore", 0.5);
	INTPOOL->Set("MyBest", 12);		// TODO


	On1PMiss = SWITCH_OFF("On1PMiss");




	//
	// Set is Autoplay or Replay or Human
	// TODO
	PlayerSwitchValue IsAutoPlay = SWITCH_OFF("IsAutoPlay");
	//
	// initalize player
	// When scene starts every time.
	//
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



	//
	// initalize note iterator (dummy)
	//
	InitalizeNote(0);
	InitalizeGauge();

	//
	// set basic play option (sudden/lift)
	// - sudden/lift first (floatspeed is relative to them)
	//
	SetSudden(playconfig->sudden);
	SetLift(playconfig->lift);
	if (playconfig->usefloatspeed) {
		SetFloatSpeed(playconfig->floatspeed);
	}
	else {
		SetSpeed(playconfig->speed);
	}
}

Player::~Player() {
	// delete note object
	SAFE_DELETE(bmsnote);
}

void Player::Reset(barindex bar) {
	// set iterator
	for (int i = 0; i < 20; i++) {
		m_Lane[i].iter_begin = m_Lane[i].iter_judge = (*bmsnote)[i].Begin(bar);
		m_Lane[i].iter_end = (*bmsnote)[i].End();
	}
}

void Player::InitalizeNote(BmsBms* bms) {
	//
	// remove note data if previously existed
	//
	if (bmsnote)
		SAFE_DELETE(bmsnote);
	bmsnote = new BmsNoteManager();

	//
	// if bms note exists, then `empty note data, default BPM`
	// otherwise, generate new note data
	//
	if (!bms) {

	}
	else {

	}
	bms->GetNoteData(*bmsnote);
	memset(&score, 0, sizeof(score));
	score.totalnote = bmsnote->GetNoteCount();

	//
	// reset TOTAL with note object
	// (IIDX style gauge TOTAL)
	//
	double total_iidx = bmsnote->GetTotalFromNoteCount();
	double total = bms->GetTotal(total_iidx);
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
	int seed = GAMESTATE.m_rseed;
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

	// initialize play speed
	speed_mul = 1;
	switch (playconfig->speedtype) {
	case SPEEDTYPE::MAXBPM:
		speed_mul = 120.0 / bms->GetTimeManager().GetMaxBPM();
		break;
	case SPEEDTYPE::MINBPM:
		speed_mul = 120.0 / bms->GetTimeManager().GetMinBPM();
		break;
	case SPEEDTYPE::MEDIUM:
		speed_mul = 120.0 / bms->GetTimeManager().GetMediumBPM();
		break;
	}

	//
	// reset iterator
	//
	Reset(0);

	//
	// clear score
	//
	score.Clear();
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

void Player::PlaySound(BmsWord& value) {
	if (!SONGVALUE.SongTime->IsStarted() || !m_PlaySound) return;
	PLAYSOUND(value);
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
		= ConvertSpeedToFloat(speed, SONGPLAYER->GetCurrentBpm());

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
	double _s = ConvertFloatToSpeed(speed, SONGPLAYER->GetCurrentBpm());
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
	// TODO: cut this function
	//

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
	if (scoregrade != GRADETYPE::GRADE_AAA) pv->pOnAAA->Stop();
	if (scoregrade != GRADETYPE::GRADE_AA) pv->pOnAA->Stop();
	if (scoregrade != GRADETYPE::GRADE_A) pv->pOnA->Stop();
	if (scoregrade != GRADETYPE::GRADE_B) pv->pOnB->Stop();
	if (scoregrade != GRADETYPE::GRADE_C) pv->pOnC->Stop();
	if (scoregrade != GRADETYPE::GRADE_D) pv->pOnD->Stop();
	if (scoregrade != GRADETYPE::GRADE_E) pv->pOnE->Stop();
	if (scoregrade != GRADETYPE::GRADE_F) pv->pOnF->Stop();
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
	//
	// (TODO)
	// fill record data more
	//
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
	// TODO: change this to class SongInfo
	double GetTimeFromBar(barindex bar) {
		return SONGPLAYER->GetBmsObject()->GetTimeManager().GetTimeFromBar(bar);
	}
}

bool Player::Lane::IsPressing() {
	return (pLaneHold && pLanePress->IsStarted());
}

bool Player::Lane::IsLongNote() {
	return (pLaneHold && pLaneHold->IsStarted());
}

BmsNote* Player::Lane::GetCurrentNote() {
	return &iter_judge->second;
}

BmsNote* Player::Lane::GetSoundNote() {
	// COMMENT: if judge note is MINE, then it won't work properly.

	if (iter_judge == iter_end) {
		if (iter_judge == iter_begin)
			return 0;
		// if no more note exists(end of the game), find previous note to play
		BmsNoteLane::Iterator iter_sound = iter_judge;
		--iter_sound;
		return &iter_sound->second;
	}
	return &iter_judge->second;
}

void Player::Lane::Next() {
	if (IsEndOfNote()) return;
	iter_judge++;
}

bool Player::Lane::IsEndOfNote() {
	return (!pLaneHold || iter_judge == iter_end);
}

void Player::UpdateBasic() {
	// get time
	Uint32 m_BmsTime = SONGVALUE.SongTime->GetTick();

	// basic timer/value update
	if (pv->pOnCombo->GetTick() > 500) pv->pOnCombo->Stop();
	if (pv->pOnMiss->GetTick() > 1000) pv->pOnMiss->Stop();
	if (pv->pOnFast->GetTick() > 500) pv->pOnFast->Stop();
	if (pv->pOnSlow->GetTick() > 500) pv->pOnSlow->Stop();
	if (pv_dp) {
		if (pv_dp->pOnCombo->GetTick() > 500) pv_dp->pOnCombo->Stop();
		if (pv_dp->pOnMiss->GetTick() > 1000) pv_dp->pOnMiss->Stop();
		if (pv_dp->pOnFast->GetTick() > 500) pv_dp->pOnFast->Stop();
		if (pv_dp->pOnSlow->GetTick() > 500) pv_dp->pOnSlow->Stop();
	}

	for (int i = 0; i < 20; i++) {
		Lane* lane = &m_Lane[i];
		BmsNote* note;

		/*
		* If (next note exists)
		* - skip all none / near-time hidden note / mine note
		*/
		while (!lane->IsEndOfNote()) {
			// skip hidden note
			note = lane->GetCurrentNote();
			if ((note->type == BmsNote::NOTE_HIDDEN && note->time - BmsJudgeTiming::POOR < m_BmsTime) ||
				(note->type == BmsNote::NOTE_MINE && note->time <= m_BmsTime) ||
				note->type == BmsNote::NOTE_NONE)
			{
				lane->Next();
			}
			else break;
		}
	}
}

/*
 * must call after song is updated
 */
void Player::Update() {
	// if gameover then don't update
	if (IsDead()) return;

	// do basic updates (MUST be called)
	UpdateBasic();

	// check for note judgement
	// COMMENT: `int i` means lane index, NOT channel number.
	for (int i = 0; i < 20; i++) {
		Lane* lane = &m_Lane[i];
		BmsNote* note;

		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 * - judge if note is too late
		 */
		while (!lane->IsEndOfNote() && (note = lane->GetCurrentNote())->time <= m_BmsTime) {
			// if note is press(HELL CHARGE), judge from pressing status
			if (note->type == BmsNote::NOTE_PRESS) {
				MakeJudge(lane->IsPressing() ? JUDGETYPE::JUDGE_PGREAT : JUDGETYPE::JUDGE_POOR,
					m_BmsTime, i);
				lane->Next();
			}
			// if not autoplay, check timing for poor
			else if (CheckJudgeByTiming(note->time - m_BmsTime)
				== JUDGETYPE::JUDGE_POOR) {
				//
				// if late, POOR judgement is always occured
				// but we need to take care of in case of LongNote
				//
				// if LNSTART late, get POOR for LNEND (2 miss)
				if (note->type == BmsNote::NOTE_LNSTART) {
					MakeJudge(JUDGETYPE::JUDGE_POOR, m_BmsTime, i);	// LNEND's poor
				}
				// if LNEND late, reset longnote pressing
				else if (note->type == BmsNote::NOTE_LNEND && lane->IsLongNote()) {
					lane->pLaneHold->Stop();
				}
				// make POOR judge
				// (CLAIM) if hidden note isn't ignored by NextAvailableNote(), 
				//         you have to hit it or you'll get miss.
				MakeJudge(JUDGETYPE::JUDGE_POOR, m_BmsTime, i);
				lane->Next();
			}
			else {
				// Don't skip note; it's not timing yet. wait next turn.
				break;
			}
		}
	}
}

// key input
void Player::UpKey(int laneidx) {
	// if gameover then ignore
	if (IsDead()) return;
	// if not DP then make lane single
	if (playmode < 10) laneidx = laneidx % 10;

	// record to replay
	Uint32 currenttime = SONGVALUE.SongTime->GetTick() + judgeoffset;
	replay_cur.AddPress(currenttime, laneidx, 0);

	// if scratch, then set timer
	// (TODO) - maybe we have to accept reverse direction.

	// convert SCDOWN to SCUP
	// (comment: in popn music, we don't do it.)
	if (laneidx == PlayerKeyIndex::P1_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P1_BUTTONSCUP;
	if (laneidx == PlayerKeyIndex::P2_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P2_BUTTONSCUP;

	// generally upkey do nothing
	// but works only if you pressing longnote
	Lane *lane = &m_Lane[laneidx];
	BmsNote* note;
	if (lane->IsLongNote() && (note = lane->GetCurrentNote())->type == BmsNote::NOTE_LNEND) {
		// get judge
		int delta = note->time - currenttime;
		int judge = CheckJudgeByTiming(delta);
		if (judge == JUDGETYPE::JUDGE_EARLY || judge == JUDGETYPE::JUDGE_NPOOR)
			judge = JUDGETYPE::JUDGE_POOR;
		MakeJudge(judge, currenttime, laneidx, delta > 0 ? 1 : 2);
		// you're not longnote anymore~
		lane->pLaneHold->Stop();
		// focus judging note to next one
		lane->Next();
	}

	// trigger time & set value
	lane->pLanePress->Stop();
	lane->pLaneUp->Start();
}

void Player::PressKey(int laneidx) {
	BmsNote *note;
	Lane *lane;

	// if gameover then ignore
	if (IsDead()) return;
	// if not DP then make lane single
	if (playmode < 10) laneidx = laneidx % 10;
	// if already pressed then ignore
	if ((lane = &m_Lane[laneidx])->IsPressing()) return;

	//
	// record to replay
	//
	Uint32 currenttime = SONGVALUE.SongTime->GetTick() + judgeoffset;
	replay_cur.AddPress(currenttime, laneidx, 1);

	// if scratch, then set timer
	// (TODO)

	// convert SCDOWN to SCUP (do it before judge)
	// (comment: in popn music, we don't do it.)
	if (laneidx == PlayerKeyIndex::P1_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P1_BUTTONSCUP;
	if (laneidx == PlayerKeyIndex::P2_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P2_BUTTONSCUP;

	/*
	 * judge process start
	 */

	// pressed!
	lane->pLanePress->Start();
	lane->pLaneUp->Stop();

	//
	// Play sound first
	// because current key index will be changed after making judgement
	//
	BmsNote *soundnote = lane->GetSoundNote();
	if (soundnote) PlaySound(soundnote->value);

	//
	// make judge
	//
	if (!lane->IsEndOfNote()) {
		note = lane->GetCurrentNote();
		int delta = note->time - currenttime;
		int fastslow = delta > 0 ? 1 : 2;
		int judge = CheckJudgeByTiming(delta);
		// only continue judging if judge isn't too fast (no judge)
		if (judge != JUDGETYPE::JUDGE_EARLY) {
			// if ÍöPOOR, then don't go to next note
			if (judge == JUDGETYPE::JUDGE_NPOOR) {
				MakeJudge(judge, currenttime, laneidx, fastslow);
			}
			// if over poor, then process and go to next note
			else {
				if (note->type == BmsNote::NOTE_LNSTART) {
					// set longnote status
					MakeJudge(judge, currenttime, laneidx, fastslow, true);
					lane->pLaneHold->Start();
				}
				else if (note->type == BmsNote::NOTE_MINE) {
					// mine damage
					if (note->value == BmsWord::MAX)
						health = 0;
					else
						health -= note->value.ToInteger();
				}
				else if (note->type == BmsNote::NOTE_NORMAL) {
					MakeJudge(judge, currenttime, laneidx, fastslow);
				}
				lane->Next();
			}
		}
	}
}

bool Player::IsDead() {
	return pv->pOnGameover->IsStarted();
}






// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto(int playside)
	: Player(playside, PLAYERTYPE::AUTO) {
	// target to 100%!
	targetrate = 1;
}

void PlayerAuto::Update() {
	// update basic things
	UpdateBasic();

	// check for note judgement
	for (int i = 0; i < 20; i++) {
		Lane* lane = &m_Lane[i];
		BmsNote* note;

		/*
		 * If (next note exists && note is at the bottom; hit timing)
		 */
		while (!lane->IsEndOfNote() && (note = lane->GetCurrentNote())->time <= m_BmsTime) {
			/*
			 * should decide what judge is proper for next
			 */
			int newjudge = JUDGETYPE::JUDGE_GOOD;
			int newexscore = (score.GetJudgedNote() + 1) * targetrate * 2 + 0.5;
			int updateexscore = newexscore - score.CalculateEXScore();
			if (updateexscore == 1) newjudge = JUDGETYPE::JUDGE_GREAT;
			else if (updateexscore >= 2) newjudge = JUDGETYPE::JUDGE_PGREAT;

			// if note is mine, then lets ignore as fast as we can
			if (note->type == BmsNote::NOTE_MINE) {
			}
			// if note is press, just combo up
			else if (note->type == BmsNote::NOTE_PRESS) {
				MakeJudge(newjudge, m_BmsTime, i);
			}
			else {
				// Autoplay -> Automatically play
				if (note->type == BmsNote::NOTE_LNSTART) {
					PlaySound(note->value);
					MakeJudge(newjudge, m_BmsTime, i, true);	// we won't display judge on LNSTART
					lane->pLaneHold->Start();
					lane->pLanePress->Start();
					lane->pLaneUp->Stop();
				}
				else if (note->type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(newjudge, m_BmsTime, i);
					lane->pLaneHold->Stop();
				}
				else if (note->type == BmsNote::NOTE_NORMAL) {
					PlaySound(note->value);
					MakeJudge(newjudge, m_BmsTime, i);
					lane->pLanePress->Start();
					lane->pLaneUp->Stop();
				}
			}

			// fetch next note
			lane->Next();
		}

		/*
		 * pressing lane is automatically up-ped
		 * about after 50ms.
		 */
		if (!lane->pLaneHold) continue;
		if (lane->pLaneHold->IsStarted())
			lane->pLanePress->Start();
		if (lane->pLaneUp->Trigger(lane->pLanePress->GetTick() > 50))
			lane->pLanePress->Stop();
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

	// also reset current ex score
	// (TODO)
}

void PlayerAuto::SetGauge(double gauge) {

}

void PlayerAuto::SetAsDead() {

}

void PlayerAuto::SetCombo(int combo) {

}

void PlayerAuto::BreakCombo() {

}






// ------ PlayerGhost ----------------------------

PlayerReplay::PlayerReplay(int playside)
	: Player(playside, PLAYERTYPE::AUTO) {}

void PlayerReplay::SetReplay(const PlayerReplayRecord &rep) {
	replay = rep;
	iter_ = replay.Begin();
}

void PlayerReplay::Update() {
	Lane* lane;
	BmsNote* note;
	UpdateBasic();

	// update judge & note press
	while (iter_ != replay.End() && iter_->time <= m_BmsTime) {
		// COMMENT: refactor this
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
			int laneidx = iter_->lane;
			lane = &m_Lane[laneidx];

			if (iter_->value == 1) {
				lane->pLaneUp->Stop();
				lane->pLanePress->Start();
			}
			else {
				lane->pLaneUp->Stop();
				lane->pLanePress->Start();
			}
			switch (lane->GetCurrentNote()->type) {
			case BmsNote::NOTE_LNSTART:
				lane->pLaneHold->Start();
				break;
			case BmsNote::NOTE_LNEND:
				lane->pLaneHold->Stop();
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
		lane = &m_Lane[i];
		// if holdnote, then lanepress -> stay active!
		if (lane->pLaneHold->IsStarted())
			lane->pLanePress->Start();
	}
}

void PlayerReplay::PressKey(int channel) {
	// do nothing
}

void PlayerReplay::UpKey(int channel) {
	// do nothing
}