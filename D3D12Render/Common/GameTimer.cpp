
#include "GameTimer.h"
#include <windows.h>

GameTimer::GameTimer() :mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mCurrentTime(0),
                        mPausedTime(0), mPrevTime(0), mStopTime(0), mStopped(false)
{
    __int64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime() const
{
    if (mStopped)
    {
        return (float)((mStopTime - mBaseTime - mPausedTime) * mSecondsPerCount);
    }
    else
    {
        return (float)((mCurrentTime - mBaseTime - mPausedTime) * mSecondsPerCount);
    }
}

float GameTimer::DeltaTime() const
{
    return (float)mDeltaTime;
}

void GameTimer::Reset()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime = currTime;
    mPrevTime = currTime;
    mStopTime = 0;
    mStopped = false;
}

void GameTimer::Start()
{
    if (mStopped)
    {
        __int64 startTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

        mPausedTime += (startTime - mStopTime);
        mPrevTime = startTime;
        mStopTime = 0;
        mStopped = false;
    }
}

void GameTimer::Stop()
{
    if (!mStopped)
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        mStopTime = currTime;
        mStopped = true;
    }
}

void GameTimer::Tick()
{
    if (mStopped)
    {
        mDeltaTime = 0.0;
        return;
    }
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    mCurrentTime = currTime;
    mDeltaTime = (mCurrentTime - mPrevTime) * mSecondsPerCount;

    if (mDeltaTime < 0.0)
    {
        mDeltaTime = 0.0;
    }

    mPrevTime = currTime;
}
