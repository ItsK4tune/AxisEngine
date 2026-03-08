#pragma once

class EngineContext;
class Scene;

class IDebugSystem
{
public:
    virtual ~IDebugSystem() = default;
    virtual void Init(EngineContext ctx) = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene& scene) = 0;
};