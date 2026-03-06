#pragma once

class IConstraint
{
public:
    virtual ~IConstraint() = default;

    virtual void SetBreakingImpulseThreshold(float threshold) = 0;
    virtual float GetBreakingImpulseThreshold() const = 0;
    
    virtual float GetAppliedImpulse() const = 0;
};
