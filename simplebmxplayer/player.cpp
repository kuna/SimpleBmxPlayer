#include "player.h"
#include "global.h"
#include "handlerargs.h"
#include "bmsresource.h"
#include "skinrendertree.h"
#include "util.h"
#include <time.h>

// OnPoorBGA

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

Grade::Grade() : Grade(0) {}
Grade::Grade(int notecnt) : notecnt(notecnt), combo(0), maxcombo(0) {
	memset(grade, 0, sizeof(grade));
}
int Grade::CalculateScore() {
	return grade[JUDGETYPE::JUDGE_PGREAT] * 2 + grade[JUDGETYPE::JUDGE_GREAT];
}
double Grade::CalculateRate() {
	return (double)CalculateScore() / notecnt / 2;
}
int Grade::CalculateGrade() {
	double rate = CalculateRate();
	if (rate >= 8.0 / 9)
		return GRADETYPE::GRADE_AAA;
	else if (rate >= 7.0 / 9)
		return GRADETYPE::GRADE_AA;
	else if (rate >= 6.0 / 9)
		return GRADETYPE::GRADE_A;
	else if (rate >= 5.0 / 9)
		return GRADETYPE::GRADE_B;
	else if (rate >= 4.0 / 9)
		return GRADETYPE::GRADE_C;
	else if (rate >= 3.0 / 9)
		return GRADETYPE::GRADE_D;
	else if (rate >= 2.0 / 9)
		return GRADETYPE::GRADE_E;
	else
		return GRADETYPE::GRADE_F;
}
void Grade::AddGrade(const int type) {
	grade[type]++;
	if (type >= JUDGETYPE::JUDGE_GOOD) {
		combo++;
		if (maxcombo < combo) maxcombo = combo;
	}
	else {
		combo = 0;
	}
}

// ------- Player ----------------------------------------

Player::Player(int type) {
	this->playertype = type;
	// initalize variables
	bmstimer = TIMERPOOL->Get("OnGameStart");
	misstimer = TIMERPOOL->Get("On1PMiss");
	memset(lanepress, 0, sizeof(lanepress));
	memset(laneup, 0, sizeof(laneup));
	memset(lanehold, 0, sizeof(lanehold));
	memset(lanejudgeokay, 0, sizeof(lanejudgeokay));

	for (int i = 0; i < 20; i++) {
		int player = i / 10 + 1;
		if (i == 6) {
			lanepress[i] = TIMERPOOL->Set(ssprintf("On%dPKeySCPress", player), false);
			laneup[i] = TIMERPOOL->Set(ssprintf("On%dPKeySCUp", player), false);
			lanehold[i] = TIMERPOOL->Set(ssprintf("On%dPJudgeSCHold", player), false);
			lanejudgeokay[i] = TIMERPOOL->Set(ssprintf("On%dPJudgeSCOkay", player), false);
		}
		else {
			lanepress[i] = TIMERPOOL->Set(ssprintf("On%dPKey%dPress", player, channel_to_lane[i]), false);
			laneup[i] = TIMERPOOL->Set(ssprintf("On%dPKey%dUp", player, channel_to_lane[i]), false);
			lanehold[i] = TIMERPOOL->Set(ssprintf("On%dPJudge%dHold", player, channel_to_lane[i]), false);
			lanejudgeokay[i] = TIMERPOOL->Set(ssprintf("On%dPJudge%dOkay", player, channel_to_lane[i]), false);
		}
		noteindex[i] = -1;
	}

	exscore_graph = DOUBLEPOOL->Get("ExScore");
	highscore_graph = DOUBLEPOOL->Get("HighScore");
	playscore = INTPOOL->Get("PlayScore");
	playmaxcombo = INTPOOL->Get("PlayMaxCombo");
	playtotalnotes = INTPOOL->Get("PlayTotalNotes");
	playgrooveguage = INTPOOL->Get("PlayGrooveGuage");
	playrivaldiff = INTPOOL->Get("PlayRivalDiff");
}

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
	else if (delta < 0)
		return JUDGETYPE::JUDGE_LATE;
	else
		return JUDGETYPE::JUDGE_EARLY;
}

bool Player::IsNoteAvailable(int notechannel) {
	return (noteindex[notechannel] >= 0);
}

int Player::GetAvailableNoteIndex(int notechannel, int start) {
	// we also ignore invisible note!
	for (int i = start; i < bmsnote[notechannel].size(); i++)
		if (bmsnote[notechannel][i].type != BmsNote::NOTE_NONE 
			&& bmsnote[notechannel][i].type != BmsNote::NOTE_HIDDEN)
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
	return &bmsnote[notechannel][noteindex[notechannel]];
}

bool Player::IsLongNote(int notechannel) {
	return longnotestartpos[notechannel] > -1;
}

void Player::Prepare(int playside) {
	// set objects and initalize
	this->playside = playside;

	// create note object
	BmsResource::BMS.GetNotes(bmsnote);

	// initalize grade instance
	grade = Grade(bmsnote.GetNoteCount());

	// TODO: depreciated, remove this.
#if 0
	for (int i = 0; i < 20; i++) {
		noteindex[i] = 0;
		longnotestartpos[i] = -1;
		// make keysound
		keysound[i].clear();
		keysound[i].resize(bmstime.GetSize());
		BmsWord lastkeysound(0);
		for (int j = 0; j < bmstime.GetSize(); j++) {
			if (bmsnote[i][j].type == BmsNote::NOTE_HIDDEN
				|| bmsnote[i][j].type == BmsNote::NOTE_LNSTART
				|| bmsnote[i][j].type == BmsNote::NOTE_NORMAL) {
				// fill whole previous notes with current keysound
				lastkeysound = bmsnote[i][j].value;
				for (int k = j; k >= 0; k--) {
					if (keysound[i][k] != BmsWord::MIN) break;
					keysound[i][k] = lastkeysound;
				}
			}
		}
		for (int j = bmstime.GetSize() - 1; j >= 0 && lastkeysound != BmsWord::MIN; j--) {
			if (keysound[i][j] != BmsWord::MIN) break;
			keysound[i][j] = lastkeysound;
		}
	}
#endif

	// initalize by finding next available note
	for (int i = 0; i < 20; i++) {
		noteindex[i] = GetAvailableNoteIndex(i);
	}
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
	// get time
	currenttime = bmstimer->GetTick();
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
				BmsNote& note = bmsnote[i][noteindex[i]];
				// if LNSTART, also kill LNEND & 2 miss
				if (note.type == BmsNote::NOTE_LNSTART) {
					note.type = BmsNote::NOTE_NONE;
					noteindex[i] = GetNextAvailableNoteIndex(i);
					MakeJudge(JUDGETYPE::JUDGE_POOR, i);
				}
				// if LNEND, reset longnotestartpos & remove LNSTART note
				else if (note.type == BmsNote::NOTE_LNEND) {
					bmsnote[i][longnotestartpos[i]].type = BmsNote::NOTE_NONE;
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
	// make judge (if you're pressing longnote)
	if (IsLongNote(keychannel)) {
		if (IsNoteAvailable(keychannel) && GetCurrentNote(keychannel)->type == BmsNote::NOTE_LNEND) {
			double t = currenttime / 1000.0;
			// make judge
			int judge = CheckJudgeByTiming(t - BmsHelper::GetCurrentTimeFromBar(noteindex[keychannel]));
			MakeJudge(judge, keychannel);
			// get next note and remove current longnote
			bmsnote[keychannel][longnotestartpos[keychannel]].type = BmsNote::NOTE_NONE;
			bmsnote[keychannel][noteindex[keychannel]].type = BmsNote::NOTE_NONE;
			noteindex[keychannel] = GetNextAvailableNoteIndex(keychannel);
		}
		longnotestartpos[keychannel] = -1;
	}
}

void Player::PressKey(int keychannel) {
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

	// make sound
	BmsHelper::PlaySound(keysound[keychannel][currentbar].ToInteger());
}

void Player::MakeJudge(int judgetype, int channel, bool silent) {
	grade.AddGrade(judgetype);
	if (judgetype >= JUDGETYPE::JUDGE_GREAT && lanejudgeokay[channel])
		lanejudgeokay[channel]->Start();
	// update graph/number
	*exscore_graph = grade.CalculateRate();
	*highscore_graph = grade.CalculateRate();
	if (!silent) {
		// TODO set timer
		switch (judgetype) {

		}
		// TODO: make judge event
		//Handler::CallHandler(OnGamePlayJudge, &judgearg);
	}
}

double Player::GetLastMissTime() {
	// TODO
	return 0;
}

double Player::GetSpeed() { return setting.speed; }

int Player::GetCurrentBar() {
	return currentbar;
}

int Player::GetCurrentNoteBar(int channel) {
	return noteindex[channel];
}

Grade Player::GetGrade() {
	return grade;
}

void Player::SetSpeed(double speed) {
	/*
	 * BPM means Beat Per Minute; that is, XXX Beats are passed during 1 minute.
	 * so 1 beat, 120BPM, screen position is : 120 / 60 / 4 = 0.5
	 * Calculating Floating Speed : (TODO)
	 * Constant speed follows time argument; (TODO)
	 */
	this->speed = speed;
	speed_mul = speed * 1.0;											// normal multiply (1x: show 4 beat in a screen)
	//speed_mul = 1.0 / speed * (120 / BmsResource::BMS.GetBaseBPM());	// if you use constant `green` speed ... (= 1 measure per 310ms)
}

bool Player::IsDead() {
	// TODO
	return false;
}

bool Player::IsFinished() {
	//
	// condition:
	// 1. guage is > HARD && health <= 0 (dead) (TODO)
	// 2. time is over (LastBar.time + POORJUDGETIME) (timeover)
	// 3. by some reason (like user cancel; same as dead) (TODO)
	//
	if (IsDead())
		return true;

	bool finished = true;

	for (int i = 0; i < 20 && finished; i++) {
		if (noteindex[i] >= 0)
			finished = false;
	}

	return finished;
}
void Player::SetPlayerSetting(const PlayerSetting& setting) {
	this->setting = setting;
}

void Player::RenderNote(SkinPlayObject *playobj) {
	double currentpos = BmsHelper::GetCurrentPosFromTime(bmstimer->GetTick() / 1000.0);
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
			switch (bmsnote[channel][currentnotebar].type) {
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

PlayerAuto::PlayerAuto() : Player(PLAYERTYPE::AUTO) {}

void PlayerAuto::Update() {
	// get time
	currenttime = bmstimer->GetTick();
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
				BmsNote& note = bmsnote[i][noteindex[i]];
				if (GetCurrentNote(i)->type == BmsNote::NOTE_LNSTART) {
					BmsHelper::PlaySound(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i, true);
					// we won't judge on LNSTART
					longnotestartpos[i] = noteindex[i];
					if (lanehold[i]) lanehold[i]->Start();
					if (lanepress[i]) {
						laneup[i]->Stop();
						lanepress[i]->Start();
					}
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_LNEND) {
					// TODO we won't play sound(turn off) on LNEND
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					longnotestartpos[i] = -1;
					if (lanehold[i]) lanehold[i]->Stop();
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_NORMAL) {
					BmsHelper::PlaySound(note.value.ToInteger());
					MakeJudge(JUDGETYPE::JUDGE_PGREAT, i);
					if (lanepress[i]) {
						laneup[i]->Stop();
						lanepress[i]->Start();
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
		if (lanepress[i] && laneup[i] && laneup[i]->Trigger(lanepress[i]->GetTick() > 50)) {
			lanepress[i]->Stop();
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

PlayerGhost::PlayerGhost() : Player(PLAYERTYPE::REPLAY) {}