#pragma once
#include <ecs/interface/i_base_system.h>



class ISkyboxService : virtual public IBaseSystem
{
public:
    virtual ~ISkyboxService() = default;
    virtual void RenderAlphaPass(Scene &scene, int width, int height, float alpha) = 0;
    virtual void RenderAlphaPassWithCamera(Scene &scene, const glm::mat4& view, const glm::mat4& proj, int width, int height, uint32_t targetFBO = 0) = 0;
};
