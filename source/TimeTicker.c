/*
	This is an abstraction around SDL_GetTicks() and works like a stop watch. 
	Unlike SDL_GetTicks(), this can count independently of the system clock, 
	which is useful for things like pausing the game.
	This also has a time scale factor which can allow for things like "bullet time".

	This is a decendant of the SClone TimeClock circa 2002.
*/

#include "TimeTicker.h"

struct TimeTicker
{
	double timeSpeed;
	Uint32 previousTime;
	Uint32 currentTime;
	SDL_bool isTickerEnabled;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

struct TimeTicker* TimeTicker_Create()
{
	struct TimeTicker* time_ticker = (struct TimeTicker*)SDL_calloc(1, sizeof(struct TimeTicker));
	TimeTicker_Reset(time_ticker);
	return time_ticker;
}



void TimeTicker_Free(struct TimeTicker* time_ticker)
{
	SDL_free(time_ticker);
}


//////////////////////////////////////////////////////////////////////
// Clock manipulating functions.
//////////////////////////////////////////////////////////////////////

// Starts/resumes the clock.
void TimeTicker_Start(struct TimeTicker* time_ticker)
{
	if(NULL == time_ticker)
	{
		return;
	}

	if( ! time_ticker->isTickerEnabled)
	{
		// We set previousTime here to SDL_GetTicks() because
		// we need a new base time to subtract from to get a 
		// difference from the next call to SDL_GetTicks().
		// This will tell us how much time elapsed between start and 
		// the next call. We can't use 0 as the base because
		// the timer may be paused at some point or the clock may not
		// start at 0.
		time_ticker->previousTime = SDL_GetTicks();
		time_ticker->isTickerEnabled = SDL_TRUE;
	}
}

// Stops the clock. Call the Start() function to resume the clock.
void TimeTicker_Stop(struct TimeTicker* time_ticker)
{
	if(time_ticker->isTickerEnabled)
	{
		// Once upon a time, I tried allowing for Stop() without
		// updating allowing for a missing delta beteen this call
		// and the last call to update(). The idea was to allow the 
		// user to pause at the moment of the last update call so 
		// the pause time could be predictably known. The missing
		// delta would then be added back to the point of resume
		// or a subsequent call to update.
		// However, this produced some bad bugs in the timer code
		// and it seems to be a messy fix at the moment.
		// So I'm giving up on the idea for now and will force 
		// an update here to account for the potential timegap
		// between the call to Stop and UpdateTime().
		TimeTicker_UpdateTime(time_ticker);
		time_ticker->isTickerEnabled = SDL_FALSE;
	}
}

// Stops the clock and sets the clock time to zero.
void TimeTicker_Reset(struct TimeTicker* time_ticker)
{
	time_ticker->isTickerEnabled = SDL_FALSE;
	time_ticker->previousTime = 0;
	time_ticker->currentTime = 0;
	time_ticker->timeSpeed = 1.0;
}


//////////////////////////////////////////////////////////////////////
// Get functions.
//////////////////////////////////////////////////////////////////////

// Gets the clock's current time in milli-seconds.
Uint32 TimeTicker_GetTime(struct TimeTicker* time_ticker)
{
	//   Uint32 value = time_ticker->isTickerEnabled ? SDL_GetTicks() : time_stop;
	//   return (Uint32)((value - time_offset) * timeSpeed);
	return time_ticker->currentTime;
}

// Updates the clock.
// Returns the new current time as a convenience.
// Use GetTime() to get the time without updating it.
Uint32 TimeTicker_UpdateTime(struct TimeTicker* time_ticker)
{
	// If the clock is on, we need to recompute the currentTime
	// Else, we just use the pre-existing currentTime
	if(SDL_TRUE == time_ticker->isTickerEnabled)
	{
		Uint32 now_time = SDL_GetTicks();
		// The amount of time that has passed is (now-previous) * timeSpeed
		// Add that to the currentTime to get the new currentTime.
		// Note the the +0.5f is for rounding when the time gets converted back to Uint32
		time_ticker->currentTime += (Uint32)(((now_time - time_ticker->previousTime) * time_ticker->timeSpeed) + 0.5);
		// Now reset the previousTime for the next time this function is called
		time_ticker->previousTime = now_time;
	}
	return time_ticker->currentTime;
}



//////////////////////////////////////////////////////////////////////
// Set functions.
//////////////////////////////////////////////////////////////////////

// Sets the speed of the clock.
// 1 is normal speed, 2 makes clock go twice as fast, 0.5 make clock go at half speed, etc.
void TimeTicker_SetSpeed(struct TimeTicker* time_ticker, double new_speed)
{
	if(new_speed >= 0)
	{
		time_ticker->timeSpeed = new_speed;
	}
}

// Sets the speed of the clock.
// 1 is normal speed, 2 makes clock go twice as fast, 0.5 make clock go at half speed, etc.
double TimeTicker_GetSpeed(struct TimeTicker* time_ticker)
{
	return time_ticker->timeSpeed;
}


