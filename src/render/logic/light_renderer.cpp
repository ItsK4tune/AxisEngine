#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <render/logic/shader.h>
#include <render/logic/light_renderer.h>
#include <render/interface/i_graphics_context.h>

#include <render/logic/shadow.h>
#include <render/interface/i_buffer_manager.h>
#include <scene/logic/scene.h>
#include <core/logic/logger.h>
#include <vector>

void LightRenderer::Init(IGraphicsContext& context)
{
    m_Context = &context;

    m_DirLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_PointLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_SpotLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());

    m_DirLights.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    m_PointLights.reserve(Shadow::MAX_POINT_LIGHTS_SHADOW * 2);
    m_SpotLights.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW * 2);
}

void LightRenderer::UploadLightData(Scene &scene, Shader *shader)
{
    if (!m_Context) return;
    auto& bm = m_Context->GetBufferManager();
    
    m_DirLights.clear();
    auto dirView = scene.registry.view<DirectionalLightComponent>();

    int dirShadowCount = 0;
    for (auto entity : dirView)
    {
        auto &light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.active)
            continue;

        float shadowIdx = -1.0f;
        if (light.isCastShadow && dirShadowCount < Shadow::MAX_DIR_LIGHTS_SHADOW)
        {
            shadowIdx = (float)dirShadowCount;
            dirShadowCount++;
        }

        glm::vec3 dir(0, -1, 0);
        if (auto* rot = scene.registry.try_get<RotationComponent>(entity))
        {
            dir = rot->value * glm::vec3(0, -1, 0);
        }

        m_DirLights.push_back({dir, shadowIdx, light.color, light.intensity});
    }

    m_PointLights.clear();
    auto pointView = scene.registry.view<PointLightComponent>();
    int pointShadowCount = 0;

    for (auto entity : pointView)
    {
        auto &light = pointView.get<PointLightComponent>(entity);
        if (!light.active)
            continue;

        float shadowIdx = -1.0f;
        if (light.isCastShadow && pointShadowCount < Shadow::MAX_POINT_LIGHTS_SHADOW)
        {
            shadowIdx = (float)pointShadowCount;
            pointShadowCount++;
        }

        glm::vec3 pos = glm::vec3(0.0f);
        if (auto* p = scene.registry.try_get<PositionComponent>(entity))
        {
            pos = p->value;
        }

        m_PointLights.push_back({pos, shadowIdx, light.color, light.intensity, light.constant, light.linear, light.quadratic, light.radius});
    }

    m_SpotLights.clear();
    auto spotView = scene.registry.view<SpotLightComponent>();
    int spotShadowCount = 0;

    for (auto entity : spotView)
    {
        auto &light = spotView.get<SpotLightComponent>(entity);
        if (!light.active)
            continue;

        float shadowIdx = -1.0f;
        if (light.isCastShadow && spotShadowCount < Shadow::MAX_SPOT_LIGHTS_SHADOW)
        {
            shadowIdx = (float)spotShadowCount;
            spotShadowCount++;
        }

        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 dir(0, -1, 0);
        auto* p = scene.registry.try_get<PositionComponent>(entity);
        auto* r = scene.registry.try_get<RotationComponent>(entity);
        if (p) pos = p->value;
        if (r) dir = r->value * glm::vec3(0, -1, 0);

        m_SpotLights.push_back({pos, 0.0f, dir, shadowIdx, light.color, light.intensity, light.cutOff, light.outerCutOff, light.constant, light.linear, light.quadratic, 0.0f, 0.0f, 0.0f});
    }

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_DirLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, (std::max)(m_DirLights.size() * sizeof(GPUDirLight), (size_t)16), m_DirLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 2, m_DirLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_PointLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, (std::max)(m_PointLights.size() * sizeof(GPUPointLight), (size_t)16), m_PointLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 3, m_PointLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_SpotLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, (std::max)(m_SpotLights.size() * sizeof(GPUSpotLight), (size_t)16), m_SpotLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, 4, m_SpotLightSSBO->Get());

    if (shader)
    {
        shader->setInt("numDirLights", (int)m_DirLights.size());
        shader->setInt("nrPointLights", (int)m_PointLights.size());
        shader->setInt("nrSpotLights", (int)m_SpotLights.size());
    }
}