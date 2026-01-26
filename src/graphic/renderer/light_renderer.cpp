#include <graphic/renderer/light_renderer.h>
#include <scene/scene.h>
#include <graphic/core/shader.h>
#include <graphic/renderer/shadow.h>
#include <vector>

void LightRenderer::Init()
{
    glGenBuffers(1, &m_DirLightSSBO);
    glGenBuffers(1, &m_PointLightSSBO);
    glGenBuffers(1, &m_SpotLightSSBO);
    
    m_DirLights.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    m_PointLights.reserve(Shadow::MAX_POINT_LIGHTS_SHADOW * 2);
    m_SpotLights.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW * 2);
}

void LightRenderer::UploadLightData(Scene &scene, Shader *shader)
{
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

        m_DirLights.push_back({dir, shadowIdx,
                             light.color, light.intensity,
                             light.ambient, 0.0f,
                             light.diffuse, 0.0f,
                             light.specular, 0.0f});
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

        m_PointLights.push_back({pos, shadowIdx,
                               light.color, light.intensity,
                               light.constant, light.linear, light.quadratic, light.radius,
                               light.ambient, 0.0f,
                               light.diffuse, 0.0f,
                               light.specular, 0.0f});
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

        m_SpotLights.push_back({pos, 0.0f,
                              dir, shadowIdx,
                              light.color, light.intensity,
                              light.cutOff, light.outerCutOff, light.constant, light.linear,
                              light.quadratic, 0.0f, 0.0f, 0.0f,
                              light.ambient, 0.0f,
                              light.diffuse, 0.0f,
                              light.specular, 0.0f});
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_DirLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_DirLights.size() * sizeof(GPUDirLight), m_DirLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_DirLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_PointLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_PointLights.size() * sizeof(GPUPointLight), m_PointLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_PointLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SpotLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_SpotLights.size() * sizeof(GPUSpotLight), m_SpotLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_SpotLightSSBO);

    shader->setInt("numDirLights", (int)m_DirLights.size());
    shader->setInt("nrPointLights", (int)m_PointLights.size());
    shader->setInt("nrSpotLights", (int)m_SpotLights.size());
}
