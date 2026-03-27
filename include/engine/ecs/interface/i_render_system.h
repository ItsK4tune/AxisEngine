#pragma once
#include <ecs/interface/i_base_system.h>

struct Scene;
class Shader;
class IRenderStateManager;

class IRenderSystem : virtual public IBaseSystem
{
public:
    virtual ~IRenderSystem() = default;
    
    virtual void Render(Scene &scene) = 0;
    virtual void RenderAlphaPass(Scene &scene, int width, int height, float alpha) {}
    virtual void RenderTransparentPass(Scene &scene, int width, int height, float alpha) {}
    virtual void RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState) {}
    virtual void RenderCapturePass(Scene &scene, int width, int height) {}
    virtual void Render(Scene &scene, Shader &shader, IRenderStateManager &renderState) {}
    virtual void RenderDebug(Scene &scene, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState) {}
};
