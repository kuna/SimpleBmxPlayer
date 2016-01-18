#include "timer.h"

namespace GameTimer {
	Uint32 globalTick;

	void Tick() {
		globalTick = SDL_GetTicks();

		/*
		 * TODO: some important timer-triggers must have to be coded here
		 * TODO: plugin can insist here - for custom timer or triggers.
		 */
	}
}

//
// original source from http://lazyfoo.net
//
Timer::Timer(int status)
{
	//Initialize the variables
	mStatus = status;
}

void Timer::Start()
{
	//Start the timer
	mStatus = TIMERSTATUS::START;

	//Get the current clock time
	mStartTick = GameTimer::globalTick;
	mPausedTick = 0;
}

void Timer::Stop()
{
	//Stop the timer
	mStatus = TIMERSTATUS::STOP;

	//Clear tick variables
	mStartTick = 0;
	mPausedTick = 0;
}

void Timer::Pause()
{
	//If the timer is running and isn't already paused
	if (mStatus == TIMERSTATUS::START)
	{
		//Pause the timer
		mStatus = TIMERSTATUS::PAUSE;

		//Calculate the paused ticks
		mPausedTick = GameTimer::globalTick - mStartTick;
		mStartTick = 0;
	}
}

void Timer::UnPause()
{
	//If the timer is running and paused
	if (mStatus = TIMERSTATUS::PAUSE)
	{
		//Unpause the timer
		mStatus = TIMERSTATUS::START;

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

	//If the timer is paused
	if (mStatus == TIMERSTATUS::PAUSE)
	{
		//Return the number of ticks when the timer was paused
		time = mPausedTick;
	}
	// or if the timer is running
	else if (mStatus == TIMERSTATUS::START)
	{
		//Return the current time minus the start time
		time = GameTimer::globalTick - mStartTick;
	}

	return time;
}

bool Timer::IsUnknown() {
	return (mStatus == TIMERSTATUS::UNKNOWN);
}

bool Timer::IsStarted()
{
	//Timer is running and paused or unpaused
	return (mStatus == TIMERSTATUS::START);
}

bool Timer::IsPaused()
{
	//Timer is running and paused
	return (mStatus == TIMERSTATUS::PAUSE);
}

bool Timer::IsStopped() {
	return (mStatus == TIMERSTATUS::STOP);
}

bool Timer::Trigger(bool condition) {
	if (!IsStarted() && condition) {
		Start();
		return true;
	}
	else {
		return false;
	}
}