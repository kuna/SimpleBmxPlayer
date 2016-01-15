#pragma once

#include "SDL/SDL_timer.h"

/*
 * General timer
 * - you have to update GameTimer::Tick() to use this class
 *   for each render event.
 */
class Timer {
private:
	Uint32 mStartTick;
	Uint32 mPausedTick;
	bool mPaused;
	bool mStarted;
public:
	Timer();
	void Tick();

	bool IsPaused();
	bool IsStarted();
	Uint32 GetTick();
	void Start();
	void Pause();
	void UnPause();
	void Stop();

	/** @brief return true when (!IsStarted() && condition) */
	bool Trigger(bool condition);
	/** @brief toggle timer. */
	void Toggle();
};

/*
 * Global timer / accessible from anywhere
 */
namespace GameTimer {
	extern Uint32 globalTick;
	void Tick();

	bool IsStarted(int n);
	// if you set forced = true, that timer will be reseted.
	void Start(int n, bool forced = false);
	void Stop(int n);
}