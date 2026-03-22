#pragma once


struct Scene;

class IDebugSystem
{
public:
    virtual ~IDebugSystem() = default;
    virtual void Initialize() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
};