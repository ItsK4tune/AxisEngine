#pragma once


class TimeService
{
public:
    virtual ~TimeService() = default;

    
    virtual float GetDeltaTime() const = 0;

    
    virtual float GetRealDeltaTime() const = 0;

    
    virtual float GetTotalTime() const = 0;

    virtual float GetTimeScale() const = 0;
    virtual void SetTimeScale(float scale) = 0;

    virtual bool IsPaused() const = 0;
    virtual void SetPaused(bool paused) = 0;

    
    virtual void SetTimeData(float dt, float realDt, float totalTime) = 0;
};

class DefaultTimeService : public TimeService
{
public:
    float GetDeltaTime() const override { return m_DeltaTime; }
    float GetRealDeltaTime() const override { return m_RealDeltaTime; }
    float GetTotalTime() const override { return m_TotalTime; }

    float GetTimeScale() const override { return m_TimeScale; }
    void SetTimeScale(float scale) override { m_TimeScale = scale; }

    bool IsPaused() const override { return m_IsPaused; }
    void SetPaused(bool paused) override { m_IsPaused = paused; }

    void SetTimeData(float dt, float realDt, float totalTime) override
    {
        m_DeltaTime = dt;
        m_RealDeltaTime = realDt;
        m_TotalTime = totalTime;
    }

private:
    float m_DeltaTime = 0.0f;
    float m_RealDeltaTime = 0.0f;
    float m_TotalTime = 0.0f;
    float m_TimeScale = 1.0f;
    bool m_IsPaused = false;
};
