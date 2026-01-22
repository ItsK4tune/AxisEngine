#include <graphic/renderer/light_renderer.h>
#include <scene/scene.h>
#include <graphic/shader.h>
#include <vector>

void LightRenderer::Init()
{
    glGenBuffers(1, &m_DirLightSSBO);
    glGenBuffers(1, &m_PointLightSSBO);
    glGenBuffers(1, &m_SpotLightSSBO);
}

void LightRenderer::UploadLightData(Scene &scene, Shader *shader)
{
    std::vector<GPUDirLight> dirLights;
    auto dirView = scene.registry.view<DirectionalLightComponent>();

    entt::entity shadowCasterEntity = entt::null;
    for (auto entity : dirView)
    {
        auto &light = dirView.get<DirectionalLightComponent>(entity);
        if (light.active && light.isCastShadow)
        {
            shadowCasterEntity = entity;
            glm::vec3 dir(0, -1, 0);
            if (scene.registry.all_of<TransformComponent>(entity))
            {
                auto &trans = scene.registry.get<TransformComponent>(entity);
                dir = trans.rotation * glm::vec3(0, -1, 0);
            }

            dirLights.push_back({dir, 0.0f,
                                 light.color, light.intensity,
                                 light.ambient, 0.0f,
                                 light.diffuse, 0.0f,
                                 light.specular, 0.0f});
            break;
        }
    }

    for (auto entity : dirView)
    {
        if (entity == shadowCasterEntity)
            continue;

        auto &light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.active)
            continue;

        glm::vec3 dir(0, -1, 0);
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            dir = trans.rotation * glm::vec3(0, -1, 0);
        }

        dirLights.push_back({dir, 0.0f,
                             light.color, light.intensity,
                             light.ambient, 0.0f,
                             light.diffuse, 0.0f,
                             light.specular, 0.0f});
    }

    std::vector<GPUPointLight> pointLights;
    auto pointView = scene.registry.view<PointLightComponent>();
    for (auto entity : pointView)
    {
        auto &light = pointView.get<PointLightComponent>(entity);
        if (!light.active)
            continue;

        glm::vec3 pos = glm::vec3(0.0f);
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
        }

        pointLights.push_back({pos, 0.0f,
                               light.color, light.intensity,
                               light.constant, light.linear, light.quadratic, light.radius,
                               light.ambient, 0.0f,
                               light.diffuse, 0.0f,
                               light.specular, 0.0f});
    }

    std::vector<GPUSpotLight> spotLights;
    auto spotView = scene.registry.view<SpotLightComponent>();
    for (auto entity : spotView)
    {
        auto &light = spotView.get<SpotLightComponent>(entity);
        if (!light.active)
            continue;

        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 dir(0, -1, 0);
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto &trans = scene.registry.get<TransformComponent>(entity);
            pos = trans.position;
            dir = trans.rotation * glm::vec3(0, -1, 0);
        }

        spotLights.push_back({pos, 0.0f,
                              dir, 0.0f,
                              light.color, light.intensity,
                              light.cutOff, light.outerCutOff, light.constant, light.linear,
                              light.quadratic, 0.0f, 0.0f, 0.0f,
                              light.ambient, 0.0f,
                              light.diffuse, 0.0f,
                              light.specular, 0.0f});
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_DirLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, dirLights.size() * sizeof(GPUDirLight), dirLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_DirLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_PointLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointLights.size() * sizeof(GPUPointLight), pointLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_PointLightSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SpotLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spotLights.size() * sizeof(GPUSpotLight), spotLights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_SpotLightSSBO);

    shader->setInt("numDirLights", (int)dirLights.size());
    shader->setInt("nrPointLights", (int)pointLights.size());
    shader->setInt("nrSpotLights", (int)spotLights.size());
}
