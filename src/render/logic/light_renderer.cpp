#include <render/logic/light_renderer.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_target_manager.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_render_service.h>
#include <render/logic/shadow_renderer.h>
#include <core/logic/service_locator.h>
#include <render/unit/shadow.h>
#include <render/interface/i_buffer_manager.h>
#include <core/logic/logger.h>
#include <cstring>
#include <algorithm>
#include <vector>

void LightRenderer::Initialize(IGraphicsContext& context)
{
    m_Context = &context;

    m_DirLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_PointLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_SpotLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());

    m_DirLights.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    m_PointLights.reserve(Shadow::MAX_POINT_LIGHTS_SHADOW * 2);
    m_SpotLights.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW * 2);
}

void LightRenderer::UploadLightData(const RenderSceneData& sceneData, Shader *shader)
{
    if (!m_Context) return;
    auto& bm = m_Context->GetBufferManager();
    
    uint32_t currentCombinedVersion = 0;
    size_t currentLightCount = sceneData.lights.size();

    for (const auto& light : sceneData.lights) {
        currentCombinedVersion += light.version;
    }

    if (currentCombinedVersion == m_LastCombinedVersion && currentLightCount == m_LastLightCount)
    {
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, 23, m_DirLightSSBO->Get());
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, 24, m_PointLightSSBO->Get());
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, 25, m_SpotLightSSBO->Get());
        return; 
    }

    m_LastCombinedVersion = currentCombinedVersion;
    m_LastLightCount = currentLightCount;

    m_DirLights.clear();
    m_PointLights.clear();
    m_SpotLights.clear();

    for (const auto& light : sceneData.lights) {
        float shadowIdx = (float)light.shadowMapIndex;

        if (light.type == RenderLightType::Directional) {
            m_DirLights.push_back({
                light.direction, shadowIdx, light.color, light.intensity,
                light.ambient, 0.0f,
                light.diffuse, 0.0f,
                light.specular, 0.0f
            });
        }
        else if (light.type == RenderLightType::Point) {
            m_PointLights.push_back({
                light.position, shadowIdx, light.color, light.intensity,
                light.constant, light.linear, light.quadratic, light.range,
                light.ambient, 0.0f,
                light.diffuse, 0.0f,
                light.specular, 0.0f
            });
        }
        else if (light.type == RenderLightType::Spot) {
            m_SpotLights.push_back({
                light.position, 0.0f, light.direction, shadowIdx,
                light.color, light.intensity, light.innerCutoff, light.outerCutoff,
                light.constant, light.linear, light.quadratic, 0.0f, 0.0f, 0.0f,
                light.ambient, 0.0f,
                light.diffuse, 0.0f,
                light.specular, 0.0f
            });
        }
    }

    auto safeSize = [](size_t count, size_t unitSize) {
        return (std::max)(count * unitSize, (size_t)16);
    };

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_DirLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_DirLights.size(), sizeof(GPUDirLight)), m_DirLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 23, m_DirLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_PointLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_PointLights.size(), sizeof(GPUPointLight)), m_PointLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 24, m_PointLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_SpotLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_SpotLights.size(), sizeof(GPUSpotLight)), m_SpotLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 25, m_SpotLightSSBO->Get());

    auto* shadowSys = ServiceLocator::Instance().Resolve<IShadowService>();
    auto* rs = ServiceLocator::Instance().Resolve<IRenderService>();
    if (shadowSys && rs)
    {
        auto& sr = shadowSys->GetRenderer();
        GPUGlobalLightData data;
        std::memcpy(data.lightSpaceMatricesDir, sr.GetLightSpaceMatrices(), sizeof(glm::mat4) * Shadow::MAX_DIR_LIGHTS_SHADOW);
        std::memcpy(data.lightSpaceMatricesSpot, sr.GetLightSpaceMatricesSpot(), sizeof(glm::mat4) * Shadow::MAX_SPOT_LIGHTS_SHADOW);
        data.numDirLights = (int)m_DirLights.size();
        data.nrPointLights = (int)m_PointLights.size();
        data.nrSpotLights = (int)m_SpotLights.size();
        data.u_ReceiveShadow = sr.IsShadowsEnabled() ? 1 : 0;
        data.farPlanePoint = sr.GetFarPlanePoint();
        data.farPlaneSpot = sr.GetFarPlaneSpot();
        data.pad0 = 0.0f;
        data.pad1 = 0.0f;
        
        rs->UpdateGlobalLightData(data);
    }
}
