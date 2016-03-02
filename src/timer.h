#pragma once

#include "SDL/SDL_timer.h"
#include "global.h"

namespace TIMERSTATUS {
	const int STOP = 0;
	const int START = 1;
	const int PAUSE = 2;
	const int UNKNOWN = 3;
}

/*
 * General timer
 * - you have to update GameTimer::Tick() to use this class
 *   for each render event.
 */
class Timer {
private:
	Uint32 mStartTick;
	Uint32 mPausedTick;
	int mStatus;
public:
	Timer(int status = TIMERSTATUS::STOP);

	// set timer as unknown state
	void SetAsUnknown();
	bool IsStopped();
	bool IsPaused();
	bool IsUnknown();
	bool IsStarted();
	Uint32 GetTick();
	void Pause();
	void UnPause();

	virtual void Start();
	virtual void Stop();
	/** @brief return true when (!IsStarted() && condition). Doesn't effect if timer is already started. */
	virtual bool Trigger(bool condition = true);
};

/*
 * Global timer / accessible from anywhere
 */
namespace GameTimer {
	extern Uint32 globalTick;
	void Tick();
}






// handlers

struct Message {
	RString name;
};

class Handler {
public:
	virtual void Receive(const Message& msg) = 0;
};

class HandlerAuto : public Handler {
public:
	HandlerAuto();
	~HandlerAuto();
	virtual void Receive(const Message& msg) = 0;
};