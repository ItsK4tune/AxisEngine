#pragma once
#include <ecs/interface/i_render_system.h>
#include <render/type/render_data.h>

struct Scene;

class ILightingService : virtual public IBaseSystem
{
public:
    virtual ~ILightingService() = default;

    virtual void RenderDeferredLighting(Scene& scene, int width, int height) = 0;
    virtual void UploadLightData(const RenderSceneData& sceneData, Shader* shader = nullptr) = 0;
};
