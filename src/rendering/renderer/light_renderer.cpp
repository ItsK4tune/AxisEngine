#include <rendering/core/shader.h>
#include <rendering/renderer/light_renderer.h>
#include <rendering/interfaces/i_graphics_context.h>

#include <rendering/renderer/shadow.h>
#include <rendering/interfaces/i_buffer_manager.h>
#include <scene/scene.h>
#include <core/utils/logger.h>
#include <vector>

void LightRenderer::Init(IGraphicsContext& context)
{
    m_Context = &context;

    m_DirLightSSBO = std::make_unique<Graphics::GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_PointLightSSBO = std::make_unique<Graphics::GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_SpotLightSSBO = std::make_unique<Graphics::GPUSSBO>(context, context.GetBufferManager().CreateBuffer());

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
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            dir = trans.rotation * glm::vec3(0, -1, 0);
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
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
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
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
            dir = trans.rotation * glm::vec3(0, -1, 0);
        }

        m_SpotLights.push_back({pos, 0.0f, dir, shadowIdx, light.color, light.intensity, light.cutOff, light.outerCutOff, light.constant, light.linear, light.quadratic, 0.0f, 0.0f, 0.0f});
    }

    bm.BindBuffer(Graphics::BufferType::ShaderStorageBuffer, m_DirLightSSBO->Get());
    bm.BufferData(Graphics::BufferType::ShaderStorageBuffer, (std::max)(m_DirLights.size() * sizeof(GPUDirLight), (size_t)16), m_DirLights.data(), Graphics::BufferUsage::DynamicDraw);
    bm.BindBufferBase(Graphics::BufferType::ShaderStorageBuffer, 0, m_DirLightSSBO->Get());

    bm.BindBuffer(Graphics::BufferType::ShaderStorageBuffer, m_PointLightSSBO->Get());
    bm.BufferData(Graphics::BufferType::ShaderStorageBuffer, (std::max)(m_PointLights.size() * sizeof(GPUPointLight), (size_t)16), m_PointLights.data(), Graphics::BufferUsage::DynamicDraw);
    bm.BindBufferBase(Graphics::BufferType::ShaderStorageBuffer, 1, m_PointLightSSBO->Get());

    bm.BindBuffer(Graphics::BufferType::ShaderStorageBuffer, m_SpotLightSSBO->Get());
    bm.BufferData(Graphics::BufferType::ShaderStorageBuffer, (std::max)(m_SpotLights.size() * sizeof(GPUSpotLight), (size_t)16), m_SpotLights.data(), Graphics::BufferUsage::DynamicDraw);
    bm.BindBufferBase(Graphics::BufferType::ShaderStorageBuffer, 2, m_SpotLightSSBO->Get());

    if (shader)
    {
        shader->setInt("numDirLights", (int)m_DirLights.size());
        shader->setInt("nrPointLights", (int)m_PointLights.size());
        shader->setInt("nrSpotLights", (int)m_SpotLights.size());
    }
}
