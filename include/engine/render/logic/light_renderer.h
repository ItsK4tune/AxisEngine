#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <render/type/render_data.h>
#include <render/type/graphics_types.h>

class IBufferManager;
class IGraphicsContext;
class Shader;

class LightRenderer
{
public:
    void Initialize(IGraphicsContext& context);
    void UploadLightData(const RenderSceneData& sceneData, Shader *shader);

    int GetDirLightCount() const { return (int)m_DirLights.size(); }
    int GetPointLightCount() const { return (int)m_PointLights.size(); }
    int GetSpotLightCount() const { return (int)m_SpotLights.size(); }

private:
    std::unique_ptr<GPUSSBO> m_DirLightSSBO;
    std::unique_ptr<GPUSSBO> m_PointLightSSBO;
    std::unique_ptr<GPUSSBO> m_SpotLightSSBO;

    std::vector<GPUDirLight> m_DirLights;
    std::vector<GPUPointLight> m_PointLights;
    std::vector<GPUSpotLight> m_SpotLights;

    uint32_t m_LastCombinedVersion = 0;
    size_t m_LastLightCount = 0;
    bool m_NeedsUpload = true;

    IGraphicsContext* m_Context = nullptr;
};