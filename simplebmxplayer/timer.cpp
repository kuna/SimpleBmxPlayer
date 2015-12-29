#include "timer.h"

namespace GameTimer {
	Uint32 globalTick;
	Uint32 mTick[10000];
	bool mStarted[10000];

	void Tick() {
		globalTick = SDL_GetTicks();

		/*
		 * TODO: some important timer-triggers must have to be coded here
		 * TODO: plugin can insist here - for custom timer or triggers.
		 */
	}
	bool IsStarted(int n) {
		return mStarted[n];
	}
	void Start(int n, bool forced) {
		if (!forced && mStarted[n])
			return;
		mStarted[n] = true;
		mTick[n] = globalTick;
	}
	void Stop(int n) {
		mStarted[n] = false;
	}
}

//
// original source from http://lazyfoo.net
//
Timer::Timer()
{
	//Initialize the variables
	mStartTick = 0;
	mPausedTick = 0;

	mPaused = false;
	mStarted = false;
}

void Timer::Start()
{
	//Start the timer
	mStarted = true;

	//Unpause the timer
	mPaused = false;

	//Get the current clock time
	mStartTick = GameTimer::globalTick;
	mPausedTick = 0;
}

void Timer::Stop()
{
	//Stop the timer
	mStarted = false;

	//Unpause the timer
	mPaused = false;

	//Clear tick variables
	mStartTick = 0;
	mPausedTick = 0;
}

void Timer::Pause()
{
	//If the timer is running and isn't already paused
	if (mStarted && !mPaused)
	{
		//Pause the timer
		mPaused = true;

		//Calculate the paused ticks
		mPausedTick = GameTimer::globalTick - mStartTick;
		mStartTick = 0;
	}
}

void Timer::UnPause()
{
	//If the timer is running and paused
	if (mStarted && mPaused)
	{
		//Unpause the timer
		mPaused = false;

		//Reset the starting ticks
		mStartTick = SDL_GetTicks() - mPausedTick;

		//Reset the paused ticks
		mPausedTick = 0;
	}
}

Uint32 Timer::GetTick()
{
	//The actual timer time
	Uint32 time = 0;

	//If the timer is running
	if (mStarted)
	{
		//If the timer is paused
		if (mPaused)
		{
			//Return the number of ticks when the timer was paused
			time = mPausedTick;
		}
		else
		{
			//Return the current time minus the start time
			time = GameTimer::globalTick - mStartTick;
		}
	}

	return time;
}

bool Timer::IsStarted()
{
	//Timer is running and paused or unpaused
	return mStarted;
}

bool Timer::IsPaused()
{
	//Timer is running and paused
	return mPaused && mStarted;
}