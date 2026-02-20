#pragma once

#include <string>

class IPhysicsBackend
{
public:
    virtual ~IPhysicsBackend() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual void StepSimulation(float dt, int maxSubSteps = 1) = 0;

    virtual void* GetWorld() = 0;
    virtual std::string GetName() const = 0;
};
