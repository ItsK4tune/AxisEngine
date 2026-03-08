#pragma once

#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <glm/glm.hpp>
#include <scene/logic/scene.h>
#include <string>

class ResourceManager;
class IRigidBody;
class ICharacterController;

#define GLM_ENABLE_EXPERIMENTAL


class EntityBuilder
{
public:
    EntityBuilder(Scene& scene, ResourceManager& resources);

    template<typename T, typename... Args>
    EntityBuilder& With(Args&&... args)
    {
        m_Scene.registry.emplace_or_replace<T>(m_Entity, std::forward<Args>(args)...);
        return *this;
    }

    EntityBuilder& WithName(const std::string& name);
    EntityBuilder& WithTag(const std::string& tag);
    EntityBuilder& WithLayer(uint32_t layer);
    
    EntityBuilder& WithTransform(const glm::vec3& pos = glm::vec3(0.0f), 
                                 const glm::vec3& rot = glm::vec3(0.0f), 
                                 const glm::vec3& scale = glm::vec3(1.0f));
                                 
    EntityBuilder& WithMesh(const std::string& modelName, const std::string& shaderName);
    EntityBuilder& WithMaterial(const MaterialComponent& material);
    EntityBuilder& WithPhongMaterial(const glm::vec3& ambient = glm::vec3(1.0f), const glm::vec3& specular = glm::vec3(0.5f), float shininess = 32.0f);
    EntityBuilder& WithPBRMaterial(float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);
    
    EntityBuilder& WithRigidBody(std::shared_ptr<IRigidBody> body);
    EntityBuilder& WithCharacterController(std::shared_ptr<ICharacterController> controller);
    
    EntityBuilder& WithUITransform(const glm::vec2& pos, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIText(const std::string& text, 
                             const std::string& fontName, 
                             float scale = 1.0f, 
                             const glm::vec3& color = glm::vec3(1.0f));

    EntityBuilder& WithAudio(const std::string& soundName, bool loop = false, float volume = 1.0f);
    EntityBuilder& WithScript(const std::string& scriptName);
    EntityBuilder& WithAnimation(const std::string& animationName);
    
    EntityBuilder& WithDirectionalLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    EntityBuilder& WithPointLight(const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f, float radius = 10.0f);
    EntityBuilder& WithSpotLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    
    EntityBuilder& WithCamera(float fov = 45.0f, float near = 0.1f, float far = 1000.0f, bool active = false);
    
    EntityBuilder& WithParticle(const std::string& particleName);
    EntityBuilder& WithVideo(const std::string& videoPath, bool loop = true);

    entt::entity Build();

private:
    Scene& m_Scene;
    ResourceManager& m_Resources;
    entt::entity m_Entity;
};
