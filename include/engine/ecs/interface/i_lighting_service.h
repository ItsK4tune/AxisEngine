#pragma once
#include <ecs/interface/i_render_system.h>

struct Scene;

/**
 * @brief Service interface for the lighting system.
 */
class ILightingService : virtual public IBaseSystem
{
public:
    virtual ~ILightingService() = default;
    
    virtual void RenderDeferredLighting(Scene &scene, int width, int height) = 0;
};
