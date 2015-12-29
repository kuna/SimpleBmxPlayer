#pragma once

#include "SDL/SDL_timer.h"

/*
 * General timer
 * - but you have to update GameTimer::Tick() to use this class
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
};

/*
 * Global timer / accessible from anywhere
 */
namespace GameTimer {
	extern Uint32 globalTick;
	void Tick();

	bool IsStarted(int n);
	void Start(int n);
	void Stop(int n);
}