#include <scene/light_manager.h>
#include <scene/scene.h>
#include <iostream>

LightManager::LightManager(Scene& scene)
    : m_Scene(scene)
{
}

LightManager::~LightManager()
{
}

entt::entity LightManager::GetPrimaryDirectionalLight() const
{
    auto view = m_Scene.registry.view<DirectionalLightComponent>();
    
    for (auto entity : view)
    {
        const auto& light = view.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            return entity;
        }
    }
    
    return entt::null;
}

std::vector<entt::entity> LightManager::GetAllDirectionalLights() const
{
    std::vector<entt::entity> lights;
    auto view = m_Scene.registry.view<DirectionalLightComponent>();
    
    for (auto entity : view)
    {
        lights.push_back(entity);
    }
    
    return lights;
}

std::vector<entt::entity> LightManager::GetAllPointLights() const
{
    std::vector<entt::entity> lights;
    auto view = m_Scene.registry.view<PointLightComponent>();
    
    for (auto entity : view)
    {
        lights.push_back(entity);
    }
    
    return lights;
}

std::vector<entt::entity> LightManager::GetAllSpotLights() const
{
    std::vector<entt::entity> lights;
    auto view = m_Scene.registry.view<SpotLightComponent>();
    
    for (auto entity : view)
    {
        lights.push_back(entity);
    }
    
    return lights;
}

std::vector<entt::entity> LightManager::GetActiveLights() const
{
    std::vector<entt::entity> lights;
    
    // Get all active directional lights
    auto dirView = m_Scene.registry.view<DirectionalLightComponent>();
    for (auto entity : dirView)
    {
        const auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (light.active)
            lights.push_back(entity);
    }
    
    // Get all active point lights
    auto pointView = m_Scene.registry.view<PointLightComponent>();
    for (auto entity : pointView)
    {
        const auto& light = pointView.get<PointLightComponent>(entity);
        if (light.active)
            lights.push_back(entity);
    }
    
    // Get all active spot lights
    auto spotView = m_Scene.registry.view<SpotLightComponent>();
    for (auto entity : spotView)
    {
        const auto& light = spotView.get<SpotLightComponent>(entity);
        if (light.active)
            lights.push_back(entity);
    }
    
    return lights;
}

void LightManager::EnsurePrimaryDirectionalLight()
{
    auto dirLightView = m_Scene.registry.view<DirectionalLightComponent>();
    bool hasShadowCaster = false;
    entt::entity lastDirLight = entt::null;
    
    for (auto entity : dirLightView)
    {
        auto& light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            hasShadowCaster = true;
            break;
        }
        if (light.active)
            lastDirLight = entity;
    }
    
    if (!hasShadowCaster && lastDirLight != entt::null)
    {
        auto& light = m_Scene.registry.get<DirectionalLightComponent>(lastDirLight);
        light.isCastShadow = true;
        std::cout << "[LightManager] Auto-set last active directional light to cast shadow" << std::endl;
    }
}

entt::entity LightManager::CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity, bool isCastShadow)
{
    entt::entity entity = m_Scene.createEntity();
    
    auto& light = m_Scene.registry.emplace<DirectionalLightComponent>(entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.isCastShadow = isCastShadow;
    light.active = true;
    
    light.ambient = color * 0.2f;
    light.diffuse = color * 0.8f;
    light.specular = glm::vec3(0.5f);
    
    return entity;
}

entt::entity LightManager::CreatePointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float radius)
{
    entt::entity entity = m_Scene.createEntity();
    
    // Set transform
    auto& transform = m_Scene.registry.emplace<TransformComponent>(entity);
    transform.position = position;
    
    // Set light
    auto& light = m_Scene.registry.emplace<PointLightComponent>(entity);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.active = true;
    
    light.ambient = color * 0.1f;
    light.diffuse = color;
    light.specular = glm::vec3(1.0f);
    
    return entity;
}

entt::entity LightManager::CreateSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float intensity, float cutOff, float outerCutOff)
{
    entt::entity entity = m_Scene.createEntity();
    
    // Set transform
    auto& transform = m_Scene.registry.emplace<TransformComponent>(entity);
    transform.position = position;
    
    // Set light
    auto& light = m_Scene.registry.emplace<SpotLightComponent>(entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.cutOff = glm::cos(glm::radians(cutOff));
    light.outerCutOff = glm::cos(glm::radians(outerCutOff));
    light.active = true;
    
    light.ambient = color * 0.1f;
    light.diffuse = color;
    light.specular = glm::vec3(1.0f);
    
    return entity;
}

void LightManager::SetPrimaryDirectionalLight(entt::entity entity)
{
    // Clear all other shadow casting flags
    auto view = m_Scene.registry.view<DirectionalLightComponent>();
    for (auto e : view)
    {
        auto& light = view.get<DirectionalLightComponent>(e);
        light.isCastShadow = false;
    }
    
    // Set new shadow caster
    if (m_Scene.registry.valid(entity) && m_Scene.registry.all_of<DirectionalLightComponent>(entity))
    {
        auto& light = m_Scene.registry.get<DirectionalLightComponent>(entity);
        light.isCastShadow = true;
    }
}

void LightManager::SetLightActive(entt::entity entity, bool active)
{
    if (!m_Scene.registry.valid(entity))
        return;
    
    if (m_Scene.registry.all_of<DirectionalLightComponent>(entity))
    {
        auto& light = m_Scene.registry.get<DirectionalLightComponent>(entity);
        light.active = active;
    }
    else if (m_Scene.registry.all_of<PointLightComponent>(entity))
    {
        auto& light = m_Scene.registry.get<PointLightComponent>(entity);
        light.active = active;
    }
    else if (m_Scene.registry.all_of<SpotLightComponent>(entity))
    {
        auto& light = m_Scene.registry.get<SpotLightComponent>(entity);
        light.active = active;
    }
}
