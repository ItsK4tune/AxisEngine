#pragma once

#include <ecs/interface/i_base_system.h>

struct Scene;
class IRenderStateManager;

class IRenderSystem : virtual public IBaseSystem {
public:
    virtual void Render(Scene& scene) {}
    virtual void RenderAlpha(Scene& scene, int width, int height, float alpha) {}
    virtual void RenderUI(Scene& scene, float width, float height, IRenderStateManager& renderState) {}
    virtual void RenderTransparent(Scene& scene, int width, int height, float alpha) {}
};
