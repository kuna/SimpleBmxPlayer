/*
 * namespace Handler
 * - you may add/call registered handler from here.
 */
#pragma once

#include "bmsbel\bms_word.h"
#define MAX_HANDLER_COUNT 1000

namespace {
	enum HANDLER {
		OnGameInitalized = 0,
		OnGamePlaySceneStarted = 1,
		OnGamePlayStarted = 2,
		OnGamePlaySound = 3,
		OnGamePlayJudge = 5,
		OnGamePlayBga = 6,
		OnGamePlayEnd = 9,
	};
}

namespace Handler {
	void AddHandler(int h, void(*f)(void*));
	void RemoveHandler(int h, void(*f)(void*));
	void CallHandler(int h, void* arg);
}

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