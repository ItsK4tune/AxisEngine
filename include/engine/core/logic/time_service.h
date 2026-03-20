#pragma once

/**
 * @brief Layer 2 service providing access to frame time data.
 * 
 * This service allows systems (Layer 4) to access delta time and real delta time
 * without depending on the application loop (Layer 5).
 */
class TimeService
{
public:
    virtual ~TimeService() = default;

    /**
     * @brief Get the delta time for the current frame (scaled).
     */
    virtual float GetDeltaTime() const = 0;

    /**
     * @brief Get the unscaled real delta time for the current frame.
     */
    virtual float GetRealDeltaTime() const = 0;

    /**
     * @brief Get the total time since engine start.
     */
    virtual float GetTotalTime() const = 0;

    virtual float GetTimeScale() const = 0;
    virtual void SetTimeScale(float scale) = 0;

    virtual bool IsPaused() const = 0;
    virtual void SetPaused(bool paused) = 0;

    /**
     * @brief Set the time data for the current frame (called by Layer 5 loop).
     */
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
