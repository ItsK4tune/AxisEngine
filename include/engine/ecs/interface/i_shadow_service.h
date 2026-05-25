#pragma once
#include <ecs/interface/i_base_system.h>

class Shadow;
class ShadowRenderer;
struct Scene;

class IShadowService : virtual public IBaseSystem
{
public:
    virtual ~IShadowService() = default;

    virtual Shadow& GetShadow() = 0;
    virtual ShadowRenderer& GetRenderer() = 0;

    virtual bool IsShadowsEnabled() const = 0;
    virtual void SetEnableShadows(bool enable) = 0;

    virtual int GetShadowMode() const = 0;
    virtual void SetShadowMode(int mode) = 0;

    virtual void PrepareShadowLights(Scene& scene) = 0;
    virtual void Render(Scene& scene) = 0;
};
