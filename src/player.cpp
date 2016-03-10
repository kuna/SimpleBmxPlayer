#include "game.h"
#include "player.h"
#include "global.h"
#include "SongPlayer.h"
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
	pSudden_d.SetFromPool(playside, "Sudden");
	pLift.SetFromPool(playside, "Lift");
	pLift_d.SetFromPool(playside, "Lift");

	pGaugeType.SetFromPool(playside, "GaugeType");
	pGauge.SetFromPool(playside, "Gauge");
	pGauge_d.SetFromPool(playside, "Gauge");
	pExscore.SetFromPool(playside, "ExScore");
	pExscore_d.SetFromPool(playside, "ExScore");
	pScore.SetFromPool(playside, "Score");
	pHighscore_d.SetFromPool(playside, "HighScore");
	pCombo.SetFromPool(playside, "Combo");
	pMaxCombo.SetFromPool(playside, "MaxCombo");
	pTotalnotes.SetFromPool(playside, "TotalNotes");
	pRivaldiff.SetFromPool(playside, "RivalDiff");
	pRate.SetFromPool(playside, "Rate");
	pRate_d.SetFromPool(playside, "Rate");
	pTotalRate.SetFromPool(playside, "TotalRate");
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

	pOnAAA.SetFromPool(playside, "AAA");
	pOnAA.SetFromPool(playside, "AA");
	pOnA.SetFromPool(playside, "A");
	pOnB.SetFromPool(playside, "B");
	pOnC.SetFromPool(playside, "C");
	pOnD.SetFromPool(playside, "D");
	pOnE.SetFromPool(playside, "E");
	pOnF.SetFromPool(playside, "F");
	pOnReachAAA.SetFromPool(playside, "ReachAAA");
	pOnReachAA.SetFromPool(playside, "ReachAA");
	pOnReachA.SetFromPool(playside, "ReachA");
	pOnReachB.SetFromPool(playside, "ReachB");
	pOnReachC.SetFromPool(playside, "ReachC");
	pOnReachD.SetFromPool(playside, "ReachD");
	pOnReachE.SetFromPool(playside, "ReachE");
	pOnReachF.SetFromPool(playside, "ReachF");

	pOnMiss.SetFromPool(playside, "Miss");
	pOnCombo.SetFromPool(playside, "Combo");
	pOnfullcombo.SetFromPool(playside, "FullCombo");
	pOnlastnote.SetFromPool(playside, "LastNote");
	pOnGameover.SetFromPool(playside, "GameOver");
	pOnGaugeMax.SetFromPool(playside, "GaugeMax");
	pOnGaugeUp.SetFromPool(playside, "GaugeUp");
	pOnMiss.SetFromPool(playside, "Miss");
	OnOptionChange.SetFromPool(playside, "OptionChange");

	/*
	* SC : note-index 0
	*/
	for (int i = 0; i < 20; i++) {
		m_Lane[i].pLanePress.SetFromPool(playside, ssprintf("Key%dPress", i));
		m_Lane[i].pLaneUp.SetFromPool(playside, ssprintf("Key%dUp", i));
		m_Lane[i].pLaneHold.SetFromPool(playside, ssprintf("Judge%dHold", i));
		m_Lane[i].pLaneOkay.SetFromPool(playside, ssprintf("Judge%dOkay", i));
	}

	// Ghost type
	// TODO: change this into lua code
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

	// Judge type
	// TODO: change this into lua code
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

	// Pacemaker
	// TODO: change this into lua code
	switch (PLAYERINFO[0].playconfig.pacemaker_type) {
	case PACEMAKERTYPE::PACE0:
		break;
	}

	// set mybest / target ex score
	DOUBLEPOOL->Set("TargetExScore", 0.5);
	INTPOOL->Set("MyBest", 12);		// TODO

	// Set is Autoplay or Replay or Human
	pAutoplay.SetFromPool(playside, "Autoplay");
	pReplay.SetFromPool(playside, "Replay");
	pHuman.SetFromPool(playside, "Human");
	pNetwork.SetFromPool(playside, "Network");
	switch (playertype) {
	case PLAYERTYPE::NETWORK:
		pNetwork.Start();
	case PLAYERTYPE::HUMAN:
		pHuman.Start();
		break;
	case PLAYERTYPE::REPLAY:
		pReplay.Start();
	case PLAYERTYPE::AUTO:
		pAutoplay.Start();
		break;
	}

	// --------------- metric setting end ------------------

	// - if autoplay | replay | network | rate < 1 | startmeasure != 0 | endmeasure < 1000
	// then don't allow save play record.
	if (playertype != PLAYERTYPE::HUMAN || GAMESTATE.m_PlayRate < 1.0
		|| GAMESTATE.m_Startmeasure != 0 || GAMESTATE.m_Endmeasure < 1000 || GAMESTATE.m_SongRepeatCount > 1)
	{
		m_IsRecordable = false;
	}
	else {
		m_IsRecordable = true;
	}

	//
	// initalize note iterator (dummy)
	//
	InitalizeGauge();	// do this first before note initalized ..?
	InitalizeNote(0);

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
	// TODO: refactor this to customizable
	//
	double total_iidx = bmsnote->GetTotalFromNoteCount();
	double total = bms->GetTotal(total_iidx);
	int notecnt = score.totalnote;
	if (notecnt <= 0)
		notecnt = 1;			// div by 0
	// TODO: set judge with total value.

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

	// TODO: reset gauge, judgement, lift/sudden information - get from profile.
	// TODO
}

/*
 * hmm little amgibuous ...
 * - coursemode: don't initalize gauge.
 */
void Player::InitalizeGauge() {
	pGaugeType
		= playergaugetype
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
		pOnGaugeMax.Trigger();
		v = 1;
	}
	else {
		pOnGaugeMax.Stop();
	}
	if (v < 0) {
		v = 0;
		pOnGameover.Trigger(dieonnohealth);
	}
	pGauge_d = playergauge = v;
	int newgauge_int = (int)(v * 50) * 2;
	if (newgauge_int > pGauge)
		pOnGaugeUp.Start();
	pGauge = newgauge_int;
}

void Player::PlaySound(BmsWord& value) {
	if (!SONGPLAYER->IsBmsLoaded() || !m_PlaySound) return;
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
	 * set metrics
	 */
	pNoteSpeed = notespeed * 100 + 0.5;
	pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	pSudden = 1000 * suddenheight;
	pLift = 1000 * liftheight;
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
	 * set metrics
	 */
	pNoteSpeed = notespeed * 100 + 0.5;
	pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	pSudden = 1000 * suddenheight;
	pLift = 1000 * liftheight;
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(notefloat + speed); }

void Player::SetSudden(double height) {
	playconfig->sudden
		= suddenheight
		= height;

	/*
	 * set metrics
	 */
	pSudden_d = height;
	pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	pSudden = 1000 * suddenheight;
	pLift = 1000 * liftheight;
}
void Player::DeltaSudden(double height) { SetSudden(suddenheight + height); }

void Player::SetLift(double height) {
	playconfig->lift
		= liftheight
		= height;

	/*
	 * set metrics
	 */
	pLift_d = height;
	pFloatSpeed = 1000 * notefloat * (1 - suddenheight - liftheight);
	pSudden = 1000 * suddenheight;
	pLift = 1000 * liftheight;
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

void Player::UpdateScore(int objtime, int channel, bool silent) {
	// calculate judgetype, fastslow from time
	int deltatime = objtime - m_BmsTime;	// this gets smaller as time goes by
	int judgetype = CheckJudgeByTiming(deltatime);
	int fastslow = 0;
	if (judgetype != BmsJudgeTiming::PGREAT) {
		if (deltatime > 0) fastslow = 1;	// fast
		else fastslow = 2;					// slow
	}
	m_curJudge.delta = deltatime;
	m_curJudge.fastslow = fastslow;
	m_curJudge.lane = channel;
	m_curJudge.silent = silent;

	// if judgetype is not valid (TOO LATE or TOO EARLY) then ignore
	if (judgetype >= 10) return;

	/*
	 * score part
	 */
	// add current judge to grade
	score.AddGrade(judgetype);
	// current pgreat
	pNotePerfect = score.score[5];
	pNoteGreat = score.score[4];
	pNoteGood = score.score[3];
	pNoteBad = score.score[2];
	pNotePoor = score.score[1] + score.score[0];

	//
	// COMMENT: cut as function from here ...?
	//

	/* fast/slow */
	const int channel_p = channel % 10;
	if (judgetype >= JUDGETYPE::JUDGE_GREAT)
		m_Lane[channel_p].pLaneOkay.Start();
	// update graph/number
	if (judgetype <= JUDGETYPE::JUDGE_BAD)
		pOnMiss.Start();
	// slow/fast?
	if (judgetype < JUDGETYPE::JUDGE_PGREAT) {
		if (fastslow == 1) {
			pOnFast.Start();
			pOnSlow.Stop();
		}
		else if (fastslow == 2) {
			pOnFast.Stop();
			pOnSlow.Start();
		}
	}
	else {
		pOnFast.Stop();
		pOnSlow.Stop();
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
	pOnReachAAA.Trigger(scoregrade >= GRADETYPE::GRADE_AAA);
	pOnReachAA.Trigger(scoregrade >= GRADETYPE::GRADE_AA);
	pOnReachA.Trigger(scoregrade >= GRADETYPE::GRADE_A);
	pOnReachB.Trigger(scoregrade >= GRADETYPE::GRADE_B);
	pOnReachC.Trigger(scoregrade >= GRADETYPE::GRADE_C);
	pOnReachD.Trigger(scoregrade >= GRADETYPE::GRADE_D);
	pOnReachE.Trigger(scoregrade >= GRADETYPE::GRADE_E);
	pOnReachF.Trigger(scoregrade >= GRADETYPE::GRADE_F);
	pOnAAA.Trigger(scoregrade == GRADETYPE::GRADE_AAA);
	pOnAA.Trigger(scoregrade == GRADETYPE::GRADE_AA);
	pOnA.Trigger(scoregrade == GRADETYPE::GRADE_A);
	pOnB.Trigger(scoregrade == GRADETYPE::GRADE_B);
	pOnC.Trigger(scoregrade == GRADETYPE::GRADE_C);
	pOnD.Trigger(scoregrade == GRADETYPE::GRADE_D);
	pOnE.Trigger(scoregrade == GRADETYPE::GRADE_E);
	pOnF.Trigger(scoregrade == GRADETYPE::GRADE_F);
	if (scoregrade != GRADETYPE::GRADE_AAA) pOnAAA.Stop();
	if (scoregrade != GRADETYPE::GRADE_AA) pOnAA.Stop();
	if (scoregrade != GRADETYPE::GRADE_A) pOnA.Stop();
	if (scoregrade != GRADETYPE::GRADE_B) pOnB.Stop();
	if (scoregrade != GRADETYPE::GRADE_C) pOnC.Stop();
	if (scoregrade != GRADETYPE::GRADE_D) pOnD.Stop();
	if (scoregrade != GRADETYPE::GRADE_E) pOnE.Stop();
	if (scoregrade != GRADETYPE::GRADE_F) pOnF.Stop();
	// TODO pacemaker
	// update gauge
	SetGauge(playergauge + notehealth[judgetype]);
	// update exscore/combo/etc ...
	const double rate_ = score.CalculateRate();
	const double rate_cur_ = score.CurrentRate();
	pExscore_d = rate_;		// graph
	pHighscore_d = rate_;		// graph
	pScore = score.CalculateScore();
	pExscore = score.CalculateEXScore();
	pRate_d = rate_cur_;
	pRate = rate_cur_ * 100;
	pMaxCombo = score.maxcombo;
	// fullcombo/lastnote check
	pOnfullcombo.Trigger(score.combo == score.totalnote);
	pOnlastnote.Trigger(score.LastNoteFinished());

	// record into playrecord
	replay_cur.AddJudge(m_BmsTime, channel, judgetype, fastslow, silent);
	
	if (!silent) {
		// show judge(combo) element
		for (int i = 0; i < 6; i++) {
			if (i == judgetype)
				pOnJudge[i].Start();
			else
				pOnJudge[i].Stop();
		}
		pOnCombo.Start();
		pCombo = score.combo;
	}

	// call update function (different from side)
	pOnCombo.Start();
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

	// save replay
	PlayerRecordHelper::SavePlayerRecord(record, player_name);
	PlayerReplayHelper::SaveReplay(
		replay_cur,
		player_name,
		current_hash
	);
}

bool Player::IsSaveable() { return m_IsRecordable; }

bool Player::IsHuman() { return playertype == PLAYERTYPE::HUMAN; }






/*------------------------------------------------------------------*
 * Game flow related
 *------------------------------------------------------------------*/

namespace {
	double GetTimeFromBar(barindex bar) {
		return SONGPLAYER->GetBmsObject()->GetTimeManager().GetTimeFromBar(bar);
	}
}

bool Lane::IsPressing() {
	return (pLanePress.IsStarted());
}

bool Lane::IsLongNote() {
	return (pLaneHold.IsStarted());
}

BmsNote* Lane::GetCurrentNote() {
	return &iter_judge->second;
}

BmsNote* Lane::GetSoundNote() {
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

void Lane::Next() {
	if (IsEndOfNote()) return;
	iter_judge++;
}

bool Lane::IsEndOfNote() {
	return (iter_judge == iter_end);
}

void Player::UpdateBasic() {
	// get time
	Uint32 m_BmsTime = SONGPLAYER->GetTick() - judgeoffset;
#if 0
	// basic timer/value update
	// (actor has it's own timer, so don't care ...?)
	if (pv->pOnCombo->GetTick() > 500) pv->pOnCombo->Stop();
	if (pv->pOnMiss->GetTick() > 1000) pv->pOnMiss->Stop();
	if (pv->pOnFast->GetTick() > 500) pv->pOnFast->Stop();
	if (pv->pOnSlow->GetTick() > 500) pv->pOnSlow->Stop();
#endif

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
				UpdateScore(lane->IsPressing() ? JUDGETYPE::JUDGE_PGREAT : JUDGETYPE::JUDGE_POOR,
					m_BmsTime, i);
				lane->Next();
			}
			// If it's later then bms timing, then judge poor
			else if (CheckJudgeByTiming(note->time - m_BmsTime)
				== JUDGETYPE::JUDGE_POOR) {
				// if LNSTART late, Set POOR for LNEND (So, totally 2 miss occured)
				if (note->type == BmsNote::NOTE_LNSTART) {
					MakeJudge(JUDGETYPE::JUDGE_POOR, m_BmsTime, i);	// LNEND's poor
				}
				// if LNEND late, reset longnote pressing
				else if (note->type == BmsNote::NOTE_LNEND && lane->IsLongNote()) {
					lane->pLaneHold.Stop();
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
		lane->pLaneHold.Stop();
		// focus judging note to next one
		lane->Next();
	}

	// trigger time & set value
	lane->pLanePress.Stop();
	lane->pLaneUp.Start();
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
	lane->pLanePress.Start();
	lane->pLaneUp.Stop();

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
					lane->pLaneHold.Start();
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
	return pOnGameover.IsStarted();
}

bool Player::IsFinished() {
	// last note or dead
	return IsDead() || score.LastNoteFinished();
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

			// if note is press, just combo up
			if (note->type == BmsNote::NOTE_PRESS) {
				UpdateJudge()
				MakeJudge(newjudge, m_BmsTime, i);
			}
			else {
				// Autoplay -> Automatically play
				if (note->type == BmsNote::NOTE_LNSTART) {
					PlaySound(note->value);
					MakeJudge(newjudge, m_BmsTime, i, true);	// we won't display judge on LNSTART
					lane->pLaneHold.Start();
					lane->pLanePress.Start();
					lane->pLaneUp.Stop();
				}
				else if (note->type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(newjudge, m_BmsTime, i);
					lane->pLaneHold.Stop();
				}
				else if (note->type == BmsNote::NOTE_NORMAL) {
					PlaySound(note->value);
					MakeJudge(newjudge, m_BmsTime, i);
					lane->pLanePress.Start();
					lane->pLaneUp.Stop();
				}
			}

			// fetch next note
			lane->Next();
		}

		/*
		 * pressing lane is automatically up-ped
		 * about after 50ms.
		 */
		if (lane->pLaneHold.IsStarted())
			lane->pLanePress.Start();
		if (lane->pLaneUp.Trigger(lane->pLanePress.GetTick() > 50))
			lane->pLanePress.Stop();
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

void PlayerReplay::SetReplay(const ReplayData &rep) {
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
			int channel = iter_->lane - 0xA0;
			// just simulate lane up / down ...
			// TODO
			//MakeJudge(s, iter_->time, playside, fastslow, silent);
			// COMMENT: in case of LN ...?
		}
		else {
			// just press / up lane
			// keysound, pressing effect should available here ...
			int laneidx = iter_->lane;
			lane = &m_Lane[laneidx];

			if (iter_->value == 1) {
				lane->pLaneUp.Stop();
				lane->pLanePress.Start();
			}
			else {
				lane->pLaneUp.Stop();
				lane->pLanePress.Start();
			}
			switch (lane->GetCurrentNote()->type) {
			case BmsNote::NOTE_LNSTART:
				lane->pLaneHold.Start();
				break;
			case BmsNote::NOTE_LNEND:
				lane->pLaneHold.Stop();
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
		if (lane->pLaneHold.IsStarted())
			lane->pLanePress.Start();
	}
}

void PlayerReplay::PressKey(int channel) {
	// do nothing
}

void PlayerReplay::UpKey(int channel) {
	// do nothing
}