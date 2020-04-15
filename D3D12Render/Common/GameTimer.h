#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:

    GameTimer();

    float TotalTime() const;    //以秒为单位
    float DeltaTime() const;    //以秒为单位
    
    void Reset();               //在开始消息循环之前调用
    void Start();               //解除计时器暂停时调用           
    void Stop();                //暂停计时器时调用
    void Tick();                //每帧都要调用

private:

    double mSecondsPerCount;
    double mDeltaTime;

    __int64 mBaseTime;
    __int64 mPausedTime;
    __int64 mStopTime;
    __int64 mPrevTime;
    __int64 mCurrentTime;

    bool mStopped;
};
#endif



