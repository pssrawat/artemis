#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <chrono>

#ifndef NUMRUNS
#define NUMRUNS 2
#endif

typedef std::chrono::high_resolution_clock HighResolutionClock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::time_point<HighResolutionClock> TimePoint;


/// Timers
static TimePoint globalTimerStart;
static TimePoint globalTimerStop;
static void startTimer()
{
  globalTimerStart = HighResolutionClock::now();
}
static void stopTimer()
{
  globalTimerStop = HighResolutionClock::now();
}
static double getElapsedTime()
{
  milliseconds time(0);
  time =
    std::chrono::duration_cast<milliseconds>
    (globalTimerStop - globalTimerStart);
  return (double)time.count();
}

#endif
