#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <vector>

class Scene;

class LightManager
{
public:
    LightManager(Scene& scene);
    ~LightManager();

    entt::entity GetPrimaryDirectionalLight() const;
    std::vector<entt::entity> GetAllDirectionalLights() const;
    std::vector<entt::entity> GetAllPointLights() const;
    std::vector<entt::entity> GetAllSpotLights() const;
    std::vector<entt::entity> GetActiveLights() const;

    void EnsurePrimaryDirectionalLight();
    
    entt::entity CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity, bool isCastShadow = false);
    entt::entity CreatePointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float radius);
    entt::entity CreateSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float intensity, float cutOff, float outerCutOff);

    void SetPrimaryDirectionalLight(entt::entity entity);
    void SetLightActive(entt::entity entity, bool active);

private:
    Scene& m_Scene;
};
