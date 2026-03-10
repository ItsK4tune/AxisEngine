#pragma once

class EngineContext;
class Scene;

class IDebugSystem
{
public:
    virtual ~IDebugSystem() = default;
    virtual void Initialize(EngineContext ctx) = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
};