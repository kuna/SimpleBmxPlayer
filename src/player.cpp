#include "game.h"
#include "player.h"
#include "global.h"
#include "Setting.h"
#include "SongPlayer.h"
#include "util.h"
#include <time.h>

// global
Player*					PLAYER[4];			// player object

// macro
#define PLAYSOUND(k)	SONGPLAYER->PlayKeySound(k)

// ------- Player ----------------------------------------

/* player should be able to work without profile information ...? */

Player::Player(int playside, int playertype) {
	this->playertype = playertype;
	this->playside = playside;

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

	pOnRank[7].SetFromPool(playside, "AAA");
	pOnRank[6].SetFromPool(playside, "AA");
	pOnRank[5].SetFromPool(playside, "A");
	pOnRank[4].SetFromPool(playside, "B");
	pOnRank[3].SetFromPool(playside, "C");
	pOnRank[2].SetFromPool(playside, "D");
	pOnRank[1].SetFromPool(playside, "E");
	pOnRank[0].SetFromPool(playside, "F");
	pOnReachRank[7].SetFromPool(playside, "ReachAAA");
	pOnReachRank[6].SetFromPool(playside, "ReachAA");
	pOnReachRank[5].SetFromPool(playside, "ReachA");
	pOnReachRank[4].SetFromPool(playside, "ReachB");
	pOnReachRank[3].SetFromPool(playside, "ReachC");
	pOnReachRank[2].SetFromPool(playside, "ReachD");
	pOnReachRank[1].SetFromPool(playside, "ReachE");
	pOnReachRank[0].SetFromPool(playside, "ReachF");

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

	// Set is Autoplay or Replay or Human
	// TODO: should be set after data is copied from profile 
	pAutoplay.SetFromPool(playside, "Autoplay");
	pReplay.SetFromPool(playside, "Replay");
	pHuman.SetFromPool(playside, "Human");
	pNetwork.SetFromPool(playside, "Network");

	// --------------- metric setting end ------------------

	//
	// initalize note iterator (dummy)
	// - So always safe to use player object.
	//
	SetNote(0);
}

Player::~Player() {
	// delete note object if exists.
	SAFE_DELETE(bmsnote);
}

void Player::SetProfile(Profile* p) {
	m_Profile = p;
	this->m_PlaySound = true;

	judgeoffset = p->config.judgeoffset;
	judgecalibration = p->config.judgecalibration;

	// - if autoplay | replay | network | rate < 1 | startmeasure != 0 | endmeasure < 1000
	// then don't allow save play record.
	m_IsRecordable = m_Option.IsAssisted();

	//
	// set basic play option (sudden/lift)
	// - sudden/lift first (floatspeed is relative to them)
	//
	SetSudden(m_Option.sudden);
	SetLift(m_Option.lift);
	if (m_Option.usefloatspeed) {
		SetFloatSpeed(m_Option.floatspeed);
	}
	else {
		SetSpeed(m_Option.speed);
	}

	// set player type
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

	/* COMMENT: gauge, judgement information is set when called SetNote() function. */
}

void Player::SetNote(BmsBms* bms) {
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
		bms->GetNoteData(*bmsnote);
	}

	// Calculate total (gauge per note)
	double total_iidx = bmsnote->GetTotalFromNoteCount();
	double total = bms->GetTotal(total_iidx);
	int notecnt = bmsnote->GetNoteCount();
	if (notecnt <= 0)
		notecnt = 1;						// prevent div by 0
	note_total = total / notecnt;

	//
	// before resetting iterator, modify note data
	//
	seed = SETTING->m_seed;					// negative seed: random seed
	if (seed < 0) seed = time(0) % 65536;
	switch (m_Option.randomS1) {			// TODO: support 2P
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

	// initialize play speed (multiplication)
	speed_mul = 1;
	switch (m_Option.speedtype) {
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

	// Call clear function since new note data is loaded
	// COMMENT: automatic call may be bad?
	Clear();
}

/*
 * MUST call after Profile & Note data is set.
 */
void Player::Clear() {
	//
	// Copy profile data, or initalize option
	//
	if (m_Profile)
		m_Option = m_Profile->option;
	else
		m_Option = PlayOption();
	// TODO: playertype?


	//
	// Reset Score
	//
	m_Score.Clear();
	m_Score.totalnote = bmsnote->GetNoteCount();

	//
	// reset TOTAL with note object
	//
	switch (m_Option.gaugetype) {
	case GAUGETYPE::GROOVE:
		gaugeval[5] = note_total / 100;
		gaugeval[4] = note_total / 100;
		gaugeval[3] = note_total / 2 / 100;
		gaugeval[2] = -2.0 / 100;
		gaugeval[1] = -6.0 / 100;
		gaugeval[0] = -2.0 / 100;
		break;
	case GAUGETYPE::EASY:
	case GAUGETYPE::ASSISTEASY:
		gaugeval[5] = note_total / 100;
		gaugeval[4] = note_total / 100;
		gaugeval[3] = note_total / 2 / 100;
		gaugeval[2] = -1.6 / 100;
		gaugeval[1] = -4.8 / 100;
		gaugeval[0] = -1.6 / 100;
		break;
	case GAUGETYPE::HARD:
		gaugeval[5] = 0.16 / 100;
		gaugeval[4] = 0.16 / 100;
		gaugeval[3] = 0;
		gaugeval[2] = -5.0 / 100;
		gaugeval[1] = -9.0 / 100;
		gaugeval[0] = -5.0 / 100;
		break;
	case GAUGETYPE::EXHARD:
		gaugeval[5] = 0.16 / 100;
		gaugeval[4] = 0.16 / 100;
		gaugeval[3] = 0;
		gaugeval[2] = -10.0 / 100;
		gaugeval[1] = -18.0 / 100;
		gaugeval[0] = -10.0 / 100;
		break;
	case GAUGETYPE::GRADE:
		gaugeval[5] = 0.16 / 100;
		gaugeval[4] = 0.16 / 100;
		gaugeval[3] = 0.04 / 100;
		gaugeval[2] = -1.5 / 100;
		gaugeval[1] = -2.5 / 100;
		gaugeval[0] = -1.5 / 100;
		break;
	case GAUGETYPE::EXGRADE:
		gaugeval[5] = 0.16 / 100;
		gaugeval[4] = 0.16 / 100;
		gaugeval[3] = 0.04 / 100;
		gaugeval[2] = -3.0 / 100;
		gaugeval[1] = -5.0 / 100;
		gaugeval[0] = -3.0 / 100;
		break;
	case GAUGETYPE::PATTACK:
		gaugeval[5] = 0;
		gaugeval[4] = 0;
		gaugeval[3] = -1;
		gaugeval[2] = -1;
		gaugeval[1] = -1;
		gaugeval[0] = -1;
		break;
	case GAUGETYPE::HAZARD:
		gaugeval[5] = 0;
		gaugeval[4] = 0;
		gaugeval[3] = 0;
		gaugeval[2] = -1;
		gaugeval[1] = -1;
		gaugeval[0] = 0;
		break;
	default:
		// copy from profile data
		memcpy(gaugeval, m_Option.gaugeval, sizeof(gaugeval));
	}

	//
	// Set Judgement value
	// http://www.powa-asso.fr/forum/viewtopic.php?f=26&t=824
	//
	// TODO: hard / extended? currently, only basic setting.
	switch (m_Option.judgetype) {
	case 0:
		judgeval[5] = 20;
		judgeval[4] = 41;
		judgeval[3] = 125;
		judgeval[2] = 173;
		judgeval[1] = 350;
		judgeval[0] = 350;
		break;
	default:
		// copy from profile data
		memcpy(judgeval, m_Option.judgeval, sizeof(judgeval));
		break;
	}

	//
	// set gaugeval
	// 
	pGaugeType
		= m_Option.gaugetype;
	switch (m_Option.gaugetype) {
	case GAUGETYPE::GROOVE:
	case GAUGETYPE::EASY:
	case GAUGETYPE::ASSISTEASY:
		SetGauge(0.2);
		m_Dieonnohealth = false;
		break;
	default:
		SetGauge(1.0);
		m_Dieonnohealth = true;
		break;
	}

	//
	// TODO update metrics
	//
	

	//
	// reset iterator
	//
	Reset(0);
}

void Player::Reset(barindex bar) {
	// set iterator
	for (int i = 0; i < 20; i++) {
		m_Lane[i].iter_begin = m_Lane[i].iter_judge = (*bmsnote)[i].Begin(bar);
		m_Lane[i].iter_end = (*bmsnote)[i].End();
	}
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
		pOnGameover.Trigger(m_Dieonnohealth);
	}
	pGauge_d = health = v;
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

void Player::RefreshFloatSpeed() {
	SetSpeed(0);
}


void Player::UpdateSpeedMetric() {
	pNoteSpeed = m_Option.speed * 100 + 0.5;
	pFloatSpeed = 1000 * m_Option.floatspeed * (1 - m_Option.sudden - m_Option.lift);
	pSudden = 1000 * m_Option.sudden;
	pLift = 1000 * m_Option.lift;
}

void Player::SetSpeed(double speed) {
	m_Option.speed
		= speed;
	m_Option.floatspeed
		= ConvertSpeedToFloat(speed, SONGPLAYER->GetCurrentBpm());
	UpdateSpeedMetric();
}
void Player::DeltaSpeed(double speed) { SetSpeed(m_Option.speed + speed); }

void Player::SetFloatSpeed(double speed) {
	double _s = ConvertFloatToSpeed(speed, SONGPLAYER->GetCurrentBpm());
	m_Option.speed
		= _s;
	m_Option.floatspeed
		= speed;
	UpdateSpeedMetric();
}
void Player::DeltaFloatSpeed(double speed) { SetFloatSpeed(m_Option.floatspeed + speed); }

void Player::SetSudden(double height) {
	m_Option.sudden
		= height;
	UpdateSpeedMetric();
}
void Player::DeltaSudden(double height) { SetSudden(m_Option.sudden + height); }

void Player::SetLift(double height) {
	m_Option.lift
		= height;
	UpdateSpeedMetric();
}
void Player::DeltaLift(double height) { SetLift(m_Option.lift + height); }

// for rendering (finally calculated note speed)
double Player::GetSpeedMul() { return m_Option.speed * speed_mul; }





/*------------------------------------------------------------------*
* Judge related
*------------------------------------------------------------------*/

int Player::GetJudgement(int delta) {
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

bool Player::AddJudgeDelta(int channel, int deltatime, bool silent) {
	// calculate judgetype, fastslow from time
	int judgetype = GetJudgement(deltatime);
	int fastslow = 0;
	if (judgetype != BmsJudgeTiming::PGREAT) {
		if (deltatime > 0) fastslow = 1;	// fast
		else fastslow = 2;					// slow
	}

	// process judge / add record.
	return AddJudge(judgetype, channel, fastslow, silent);
}

/* next iteration: true, else: false */
bool Player::AddJudge(int channel, int judgetype, int fastslow, bool silent) {
	// early judge can't effect any of them ...
	if (judgetype == JUDGETYPE::JUDGE_EARLY)
		return false;

	// add current judge to grade
	m_Score.AddGrade(judgetype);

	// update metrics: current pgreat
	pNotePerfect = m_Score.score[5];
	pNoteGreat = m_Score.score[4];
	pNoteGood = m_Score.score[3];
	pNoteBad = m_Score.score[2];
	pNotePoor = m_Score.score[1] + m_Score.score[0];

	/* fast/slow */
	const int channel_p = channel % 10;
	if (judgetype >= JUDGETYPE::JUDGE_GREAT)
		m_Lane[channel_p].pLaneOkay.Start();
	// update metrics: graph/number
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
			m_Score.fast++;
			if (judgecalibration) judgeoffset++;
		}
		else if (fastslow == 2) {
			m_Score.slow++;
			if (judgecalibration) judgeoffset--;
		}
	}
	// reached rank?
	const int scoregrade = m_Score.CalculateGrade();
	for (int i = 0; i < 8; i++) {
		pOnReachRank[i].Trigger(scoregrade >= i);
		if (scoregrade == i)
			pOnRank[i].Trigger();
		else
			pOnRank[i].Stop();
	}
	// update gauge
	SetGauge(health + gaugeval[judgetype]);
	// update exscore/combo/etc ...
	const double rate_ = m_Score.CalculateRate();
	const double rate_cur_ = m_Score.CurrentRate();
	pExscore_d = rate_;		// graph
	pHighscore_d = rate_;		// graph
	pScore = m_Score.CalculateScore();
	pExscore = m_Score.CalculateEXScore();
	pRate_d = rate_cur_;
	pRate = rate_cur_ * 100;
	pMaxCombo = m_Score.maxcombo;
	// fullcombo/lastnote check
	pOnfullcombo.Trigger(m_Score.combo == m_Score.totalnote);
	pOnlastnote.Trigger(m_Score.LastNoteFinished());

	// record into playrecord
	m_ReplayRec.AddJudge(m_BmsTime, channel, judgetype, fastslow, silent);

	// if note silent then show judge(combo) element
	if (!silent) {
		for (int i = 0; i < 6; i++) {
			if (i == judgetype)
				pOnJudge[i].Start();
			else
				pOnJudge[i].Stop();
		}
		pOnCombo.Start();
		pCombo = m_Score.combo;
	}

	// cache current judge before trigger handler
	m_curJudge.fastslow = fastslow;
	m_curJudge.lane = channel;
	m_curJudge.silent = silent;

	// call update function (different from side)
	pOnCombo.Start();

	// automatically go to next iterator, as judgement and note is 1:1 corresponds
	// COMMENT: in back-spin scratch, we need to stop iterator (just seek)...?
	if (judgetype >= JUDGETYPE::JUDGE_POOR) {
		m_Lane[channel].Next();
		return true;
	} else return false;
}

void Player::Save() {
	if (!m_Profile) return;

	// Copy changed profile option
	m_Profile->option = m_Option;

	// TODO fill record data
	RString current_hash = SETTING->m_CourseHash[SETTING->m_CourseRound];
	PlayRecord m_PlayRec;
	m_PlayRec.hash = current_hash;
	m_Profile->LoadSongRecord(current_hash, m_PlayRec);
	if (IsDead() || (!m_Dieonnohealth && health < 0.8))
		m_PlayRec.failcount++;
	else
		m_PlayRec.clearcount++;
	m_PlayRec.maxcombo = m_Score.maxcombo;
	m_PlayRec.minbp = m_Score.score[0] + m_Score.score[1] + m_Score.score[2];
	m_PlayRec.cbrk = m_Score.cbrk;
	m_PlayRec.score = m_Score;

	// save replay
	m_Profile->SaveReplayData(current_hash, m_ReplayRec);
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
				AddJudge(i, lane->IsPressing() ? JUDGETYPE::JUDGE_PGREAT : JUDGETYPE::JUDGE_POOR, 0);
			}
			// If it's later then bms timing, then judge poor
			else if (GetJudgement(note->time - m_BmsTime)
				== JUDGETYPE::JUDGE_POOR) {
				// if LNSTART late, Set POOR for LNEND (So, totally 2 miss occured)
				if (note->type == BmsNote::NOTE_LNSTART) {
					AddJudge(i, JUDGETYPE::JUDGE_POOR, 0);	// LNEND's poor
				}
				// if LNEND late, reset longnote pressing
				else if (note->type == BmsNote::NOTE_LNEND && lane->IsLongNote()) {
					lane->pLaneHold.Stop();
				}
				// make POOR judge
				// (CLAIM) if hidden note isn't ignored by NextAvailableNote(), 
				//         you have to hit it or you'll get miss.
				AddJudge(i, JUDGETYPE::JUDGE_POOR, 0);
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
	Uint32 currenttime = SONGPLAYER->GetTick() + judgeoffset;
	m_ReplayRec.AddPress(currenttime, laneidx, 0);

	// if scratch, then set timer
	// (TODO) - maybe we have to accept reverse direction.

	// convert SCDOWN to SCUP
	// (comment: in popn music, we don't do it.)
	// COMMENT: make UP -> DOWN, rather?
	if (laneidx == PlayerKeyIndex::P1_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P1_BUTTONSCUP;
	if (laneidx == PlayerKeyIndex::P2_BUTTONSCDOWN) laneidx = PlayerKeyIndex::P2_BUTTONSCUP;

	// generally upkey do nothing
	// but works only if you pressing longnote
	Lane *lane = &m_Lane[laneidx];
	BmsNote* note;
	if (lane->IsLongNote() && (note = lane->GetCurrentNote())->type == BmsNote::NOTE_LNEND) {
		// get judge
		int delta = note->time - currenttime;
		int judge = GetJudgement(delta);
		// if you UP note too early, then poor judgement.
		if (judge == JUDGETYPE::JUDGE_EARLY || judge == JUDGETYPE::JUDGE_NPOOR)
			judge = JUDGETYPE::JUDGE_POOR;
		AddJudge(laneidx, judge, delta < 0 ? 2 : 1);
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
	Uint32 currenttime = SONGPLAYER->GetTick() + judgeoffset;
	m_ReplayRec.AddPress(currenttime, laneidx, 1);

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
		int judge = GetJudgement(delta);
		// only continue judging if judge isn't too fast (no judge)
		if (judge != JUDGETYPE::JUDGE_EARLY) {
			// if BOMB note then change health
			if (note->type == BmsNote::NOTE_MINE) {
				// change judge to NPOOR (POOR is counted to note count)
				judge = JUDGETYPE::JUDGE_NPOOR;
				// mine damage
				if (note->value == BmsWord::MAX)
					health = 0;
				else
					health -= note->value.ToInteger();
			}
			// if not ÍöPOOR (valid note; next iteration)
			else if (AddJudge(laneidx, judge, fastslow)) {
				if (note->type == BmsNote::NOTE_LNSTART) {
					// if judge okay(COMMENT), then set longnote
					lane->pLaneHold.Start();
				}
			}
		}
	}
}

bool Player::IsDead() {
	return pOnGameover.IsStarted();
}

bool Player::IsFinished() {
	// last note or dead
	return IsDead() || m_Score.LastNoteFinished();
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
			int newexscore = (m_Score.GetJudgedNote() + 1) * targetrate * 2 + 0.5;
			int updateexscore = newexscore - m_Score.CalculateEXScore();
			if (updateexscore == 1) newjudge = JUDGETYPE::JUDGE_GREAT;
			else if (updateexscore >= 2) newjudge = JUDGETYPE::JUDGE_PGREAT;
			bool silent = false;

			// Autoplay -> Automatically play
			if (note->type == BmsNote::NOTE_LNSTART) {
				PlaySound(note->value);
				silent = true;
				lane->pLaneHold.Start();
				lane->pLanePress.Start();
				lane->pLaneUp.Stop();
			}
			else if (note->type == BmsNote::NOTE_LNEND) {
				// TODO we won't play sound(turn off) on LNEND
				lane->pLaneHold.Stop();
			}
			else if (note->type == BmsNote::NOTE_NORMAL) {
				PlaySound(note->value);
				lane->pLanePress.Start();
				lane->pLaneUp.Stop();
			}

			// process judge
			AddJudge(i, newjudge, 0, silent);
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