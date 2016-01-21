/*
 * @description definitions of arguments used for global handlers
 */
#pragma once

#include "bmsbel\bms_word.h"
#define MAX_HANDLER_COUNT 1000

// for handler argument
struct OnGamePlaySoundArg {
	BmsWord value;
	int on;
	OnGamePlaySoundArg(BmsWord value, int on) : value(value), on(on) {}
};

struct OnGamePlayJudgeArg {
	int judge, playside;
	OnGamePlayJudgeArg(int judge, int playside) : judge(judge), playside(playside) {}
};

struct OnGamePlayBgaArg {
	BmsWord value;
	int bgatype;
	OnGamePlayBgaArg(BmsWord value, int bgatype) : value(value), bgatype(bgatype) {}
};