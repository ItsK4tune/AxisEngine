#pragma once
#include <ecs/interface/i_base_system.h>

class Scene;
class Shader;
class IRenderStateManager;

/**
 * @brief Interface for systems that perform rendering operations.
 */
class IRenderSystem : virtual public IBaseSystem
{
public:
    virtual ~IRenderSystem() = default;
    
    virtual void Render(Scene &scene) = 0;
    virtual void RenderAlpha(Scene &scene, int width, int height, float alpha) {}
    virtual void RenderTransparent(Scene &scene, int width, int height, float alpha) {}
    virtual void RenderUI(Scene &scene, float width, float height, IRenderStateManager &renderState) {}

    // Specific rendering with shader
    virtual void Render(Scene &scene, Shader &shader, IRenderStateManager &renderState) {}

    /**
     * @brief Systems that render debug overlays.
     */
    virtual void RenderDebug(Scene &scene, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState) {}
};
