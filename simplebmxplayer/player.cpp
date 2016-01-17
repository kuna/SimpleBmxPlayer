#include "player.h"
#include "global.h"
#include "handlerargs.h"
#include "bmsresource.h"
#include <time.h>

const int Grade::JUDGE_PGREAT = 5;
const int Grade::JUDGE_GREAT = 4;
const int Grade::JUDGE_GOOD = 3;
const int Grade::JUDGE_POOR = 2;
const int Grade::JUDGE_BAD = 1;
const int Grade::JUDGE_EARLY = 10;
const int Grade::JUDGE_LATE = 11;

const int Grade::GRADE_AAA = 8;
const int Grade::GRADE_AA = 7;
const int Grade::GRADE_A = 6;
const int Grade::GRADE_B = 5;
const int Grade::GRADE_C = 4;
const int Grade::GRADE_D = 3;
const int Grade::GRADE_E = 2;
const int Grade::GRADE_F = 1;

Grade::Grade() : Grade(0) {}
Grade::Grade(int notecnt): notecnt(notecnt) {}
int Grade::CalculateScore() {
	return grade[Grade::JUDGE_PGREAT] * 2 + grade[Grade::JUDGE_GREAT];
}
double Grade::CalculateRate() {
	return (double)CalculateScore() / notecnt * 100;
}
int Grade::CalculateGrade() {
	double rate = CalculateRate();
	if (rate >= 8.0 / 9)
		return Grade::GRADE_AAA;
	else if (rate >= 7.0 / 9)
		return Grade::GRADE_AA;
	else if (rate >= 6.0 / 9)
		return Grade::GRADE_A;
	else if (rate >= 5.0 / 9)
		return Grade::GRADE_B;
	else if (rate >= 4.0 / 9)
		return Grade::GRADE_C;
	else if (rate >= 3.0 / 9)
		return Grade::GRADE_D;
	else if (rate >= 2.0 / 9)
		return Grade::GRADE_E;
	else
		return Grade::GRADE_F;
}
void Grade::AddGrade(const int type) {
	grade[type]++;
	if (type >= Grade::JUDGE_GOOD) {
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
	for (int i = 0; i < 20; i++) {
		noteindex[i] = -1;
	}
}

int Player::CheckJudgeByTiming(double delta) {
	double abs_delta = abs(delta);
	if (abs_delta <= BmsJudgeTiming::PGREAT)
		return Grade::JUDGE_PGREAT;
	else if (abs_delta <= BmsJudgeTiming::GREAT)
		return Grade::JUDGE_GREAT;
	else if (abs_delta <= BmsJudgeTiming::GOOD)
		return Grade::JUDGE_GOOD;
	else if (abs_delta <= BmsJudgeTiming::BAD)
		return Grade::JUDGE_BAD;
	else if (abs_delta <= BmsJudgeTiming::POOR)
		return Grade::JUDGE_POOR;
	else if (delta < 0)
		return Grade::JUDGE_LATE;
	else
		return Grade::JUDGE_EARLY;
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

void Player::Prepare(BmsBms* bms, int playside) {
	// set objects and initalize
	this->bms = bms;
	this->playside = playside;

	// create note object
	bms->GetNotes(bmsnote);

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
	currenttime = gametimer->GetTick();
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
			else if (CheckJudgeByTiming(bmstime.GetRow(noteindex[i]).time - time_sec) == Grade::JUDGE_LATE) {
				BmsNote& note = bmsnote[i][noteindex[i]];
				// if LNSTART, also kill LNEND & 2 miss
				if (note.type == BmsNote::NOTE_LNSTART) {
					note.type = BmsNote::NOTE_NONE;
					noteindex[i] = GetNextAvailableNoteIndex(i);
					grade.AddGrade(Grade::JUDGE_POOR);
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
				grade.AddGrade(Grade::JUDGE_POOR);
				OnGamePlayJudgeArg handler_arg(Grade::JUDGE_POOR, note.channel<10 ? 0 : 1);
				//Handler::CallHandler(OnGamePlayJudge, &handler_arg);
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
			int judge = CheckJudgeByTiming(t - bmstime.GetRow(noteindex[keychannel]).time);
			OnGamePlayJudgeArg handler_arg(judge, keychannel<10 ? 0 : 1);
			// make judge
			//Handler::CallHandler(OnGamePlayJudge, &handler_arg);
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
		int judge = CheckJudgeByTiming(t - bmstime.GetRow(noteindex[keychannel]).time);
		if (GetCurrentNote(keychannel)->type == BmsNote::NOTE_LNSTART) {
			// store longnote start pos & set longnote status
			grade.AddGrade(judge);
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
			// judge
			grade.AddGrade(judge);
			OnGamePlayJudgeArg handler_arg(Grade::JUDGE_POOR, keychannel<10 ? 0 : 1);
			// make judge
			//Handler::CallHandler(OnGamePlayJudge, &handler_arg);
		}
	}

	// make sound
	BmsHelper::PlaySound(keysound[keychannel][currentbar].ToInteger());
}

double Player::GetLastMissTime() {
	// TODO
	return 0;
}

double Player::GetSpeed() { return setting.speed; }

double Player::GetCurrentPos() {
	// calculate abspos
	return bmstime.GetAbsBeatFromTime(currenttime / 1000.0);
}

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

// ------ PlayerAuto -----------------------------------

PlayerAuto::PlayerAuto() : Player(PLAYERTYPE::AUTO) {}

void PlayerAuto::Update() {
	// get time
	currenttime = gametimer->GetTick();
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
				continue;
			}
			else {
				// Autoplay -> Automatically play
				BmsNote& note = bmsnote[i][noteindex[i]];
				if (GetCurrentNote(i)->type == BmsNote::NOTE_LNSTART) {
					BmsHelper::PlaySound(note.value.ToInteger());
					grade.AddGrade(Grade::JUDGE_PGREAT);
					// we won't judge on LNSTART
					longnotestartpos[i] = noteindex[i];
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_LNEND) {
					// we won't play sound(turn off) on LNEND
					OnGamePlaySoundArg soundarg(note.value, 0);
					//Handler::CallHandler(OnGamePlaySound, &soundarg);
					grade.AddGrade(Grade::JUDGE_PGREAT);
					OnGamePlayJudgeArg judgearg(Grade::JUDGE_PGREAT, note.channel<10 ? 0 : 1);
					// TODO: make judge event
					//Handler::CallHandler(OnGamePlayJudge, &judgearg);
					longnotestartpos[i] = -1;
				}
				else if (GetCurrentNote(i)->type == BmsNote::NOTE_NORMAL) {
					OnGamePlaySoundArg soundarg(note.value, 1);
					//Handler::CallHandler(OnGamePlaySound, &soundarg);
					grade.AddGrade(Grade::JUDGE_PGREAT);
					OnGamePlayJudgeArg judgearg(Grade::JUDGE_PGREAT, note.channel<10 ? 0 : 1);
					// TODO: make judge event
					//Handler::CallHandler(OnGamePlayJudge, &judgearg);
				}
			}

			// fetch next note
			noteindex[i] = GetNextAvailableNoteIndex(i);
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