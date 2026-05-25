#pragma once

#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <scene/logic/scene.h>
#include <glm/glm.hpp>
#include <string>

class ResourceManager;
class IRigidBody;
class ICharacterController;
struct AxisMaterialComponent;

#define GLM_ENABLE_EXPERIMENTAL

class EntityBuilder
{
public:
    EntityBuilder(Scene& scene, ResourceManager& resources, const std::string& sceneName);

    template <typename T, typename... Args>
    EntityBuilder& With(Args&&... args)
    {
        m_Scene.registry.emplace_or_replace<T>(m_Entity, std::forward<Args>(args)...);
        return *this;
    }

    EntityBuilder& WithName(const std::string& name);
    EntityBuilder& WithTag(const std::string& tag);
    EntityBuilder& WithLayer(uint32_t layer);
    EntityBuilder& WithScene(const std::string& sceneName);

    EntityBuilder& WithTransform(const glm::vec3& pos = glm::vec3(0.0f), const glm::vec3& rot = glm::vec3(0.0f),
                                 const glm::vec3& scale = glm::vec3(1.0f));

    EntityBuilder& WithMesh(const std::string& modelName, const std::string& shaderName);
    EntityBuilder& WithMaterial(const AxisMaterialComponent& material);
    EntityBuilder& WithPhongMaterial(const glm::vec3& ambient = glm::vec3(1.0f),
                                     const glm::vec3& specular = glm::vec3(0.5f), float shininess = 32.0f);
    EntityBuilder& WithPBRMaterial(float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);

    EntityBuilder& WithRigidBody(std::shared_ptr<IRigidBody> body);
    EntityBuilder& WithCharacterController(std::shared_ptr<ICharacterController> controller);

    EntityBuilder& WithPathFollower(float moveSpeed = 5.0f, float rotationSpeed = 10.0f, float maxRotationSpeed = 20.0f,
                                    float rotationAcceleration = 40.0f,
                                    const glm::vec3& rotationOffset = glm::vec3(0.0f));

    EntityBuilder& WithUITransform(const glm::vec2& pos, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIAnchors(const glm::vec2& min, const glm::vec2& max);
    EntityBuilder& WithUIOffsets(const glm::vec2& min, const glm::vec2& max);
    EntityBuilder& WithUIPivot(const glm::vec2& pivot);
    EntityBuilder& WithUIFlex(FlexDirection dir, float spacing = 5.0f);
    EntityBuilder& WithUIText(const std::string& text, const std::string& fontName, float scale = 1.0f,
                              const glm::vec4& color = glm::vec4(1.0f));
    EntityBuilder& WithUITextAlignment(TextAlignment align, bool wrap = false, float maxWidth = 0.0f);
    EntityBuilder& WithUIRenderer(const std::string& textureName, const glm::vec4& color = glm::vec4(1.0f));
    EntityBuilder& WithParent(entt::entity parent);
    EntityBuilder& WithAudio(const std::string& soundName, bool loop = false, float volume = 1.0f);
    EntityBuilder& WithScript(const std::string& scriptName);
    EntityBuilder& WithAnimation(const std::string& animationName);

    EntityBuilder& WithDirectionalLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f),
                                        float intensity = 1.0f);
    EntityBuilder& WithPointLight(const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f,
                                  float radius = 10.0f);
    EntityBuilder& WithSpotLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f),
                                 float intensity = 1.0f, float radius = 50.0f);

    EntityBuilder& WithCamera(float fov = 45.0f, float near = 0.1f, float far = 1000.0f, bool active = false);

    EntityBuilder& WithParticle(const std::string& particleName);
    EntityBuilder& WithVideo(const std::string& videoPath, bool loop = true);

    entt::entity Build();

private:
    Scene& m_Scene;
    ResourceManager& m_Resources;
    entt::entity m_Entity;
};
