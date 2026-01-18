#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <irrKlang/irrKlang.h>

#include <btBulletDynamicsCommon.h>
#include <graphic/model.h>
#include <graphic/ui_model.h>
#include <graphic/animator.h>
#include <graphic/font.h>
#include <graphic/skybox.h>
#include <graphic/particle_emitter.h>

struct InfoComponent
{
    std::string name = "Entity";
    std::string tag = "Default";
};

struct TransformComponent
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    entt::entity parent = entt::null;
    std::vector<entt::entity> children;

    glm::mat4 GetLocalModelMatrix() const;
    
    glm::mat4 GetWorldModelMatrix(entt::registry& registry) const;

    void SetParent(entt::entity thisEntity, entt::entity newParent, entt::registry& registry, bool keepWorldTransform = false);
    void AddChild(entt::entity thisEntity, entt::entity child, entt::registry& registry, bool keepWorldTransform = false);
    void RemoveChild(entt::entity child);
    bool HasParent() const { return parent != entt::null; }
    uint32_t GetVersion() const { return m_Version; }

private:
    mutable glm::mat4 m_LocalMatrix = glm::mat4(1.0f);
    mutable glm::mat4 m_WorldMatrix = glm::mat4(1.0f);
    
    mutable glm::vec3 m_LastPosition = glm::vec3(0.0f);
    mutable glm::quat m_LastRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    mutable glm::vec3 m_LastScale = glm::vec3(1.0f);
    
    mutable uint32_t m_Version = 0;
    mutable uint32_t m_LastParentVersion = 0;
    mutable entt::entity m_LastParent = entt::null;
    mutable uint32_t m_LastLocalVersion = 0;
};

struct MeshRendererComponent
{
    Model *model = nullptr;
    Shader *shader = nullptr;
    bool castShadow = true;
    glm::vec4 color = glm::vec4(1.0f);
};

enum class MaterialType
{
    PHONG,
    PBR
};

struct MaterialComponent
{
    MaterialType type = MaterialType::PHONG;

    // Common
    float roughness = 0.5f; 
    float opacity = 1.0f;
    glm::vec3 emission = glm::vec3(0.0f);

    // Phong
    float shininess = 32.0f;
    glm::vec3 specular = glm::vec3(0.5f);
    glm::vec3 ambient = glm::vec3(1.0f);

    // PBR
    float metallic = 0.0f;
    float ao = 1.0f;

    // Mapping
    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f);
};

struct RigidBodyComponent
{
    btRigidBody *body = nullptr;
    btTypedConstraint* constraint = nullptr; // For parent attachment
    bool isAttachedToParent = false;

    void SetRestitution(float restitution) {
        if (body) body->setRestitution(restitution);
    }
    
    void SetFriction(float friction) {
        if (body) body->setFriction(friction);
    }

    void SetLinearFactor(const glm::vec3& factor) {
        if (body) body->setLinearFactor(btVector3(factor.x, factor.y, factor.z));
    }

    void SetAngularFactor(const glm::vec3& factor) {
        if (body) body->setAngularFactor(btVector3(factor.x, factor.y, factor.z));
    }
    
    void SetLinearVelocity(const glm::vec3& vel) {
        if (body) {
            body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
            body->activate(true);
        }
    }

    void SetAngularVelocity(const glm::vec3& vel) {
        if (body) {
            body->setAngularVelocity(btVector3(vel.x, vel.y, vel.z));
            body->activate(true);
        }
    }
};

struct AnimationComponent
{
    Animator *animator = nullptr;
};

struct CameraComponent
{
    bool isPrimary = false;

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float aspectRatio = 1.0f;

    float yaw = -90.0f;
    float pitch = 0.0f;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
};

struct UITransformComponent
{
    glm::vec2 position;
    glm::vec2 size;
    int zIndex = 0;
};

struct UIRendererComponent
{
    UIModel *model = nullptr;
    Shader *shader = nullptr;
    glm::vec4 color = glm::vec4(1.0f);
};

struct UIInteractiveComponent
{
    bool isHovered = false;
    bool isPressed = false;

    std::function<void(entt::entity)> onClick;
    std::function<void(entt::entity)> onHoverEnter;
    std::function<void(entt::entity)> onHoverExit;
};

struct UIAnimationComponent
{
    bool isAnimating = false;

    float targetScale = 1.0f;
    float currentScale = 1.0f;
    float speed = 5.0f;

    glm::vec4 hoverColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    glm::vec4 normalColor = glm::vec4(1.0f);
};

struct UITextComponent
{
    UIModel *model = nullptr;
    Shader *shader = nullptr;
    std::string text;
    Font *font = nullptr;
    glm::vec3 color = glm::vec3(1.0f);
    float scale = 1.0f;
    // float padding/lineHeight... (nâng cao)
};

struct DirectionalLightComponent
{
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(0.4f);
    glm::vec3 specular = glm::vec3(0.5f);
};

struct PointLightComponent
{
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    float radius = 10.0f;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);
};

struct SpotLightComponent
{
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;

    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);
};

class Scriptable;

struct ScriptComponent
{
    Scriptable *instance = nullptr;

    std::function<Scriptable *()> InstantiateScript;
    std::function<void(ScriptComponent *)> DestroyScript; 

    template <typename T>
    void Bind()
    {
        InstantiateScript = []()
        { return static_cast<Scriptable *>(new T()); };
        DestroyScript = [](ScriptComponent *nsc)
        { delete nsc->instance; nsc->instance = nullptr; };
    }
};

struct AudioSourceComponent
{
    std::string filePath;
    float volume = 1.0f;
    bool loop = false;
    bool is3D = true;
    bool playOnAwake = true;
    float minDistance = 1.0f;

    irrklang::ISound* sound = nullptr;
    bool shouldPlay = false; // Trigger to play
};

struct SkyboxRenderComponent
{
    Skybox *skybox;
    Shader *shader;
};

struct ParticleEmitterComponent
{
    ParticleEmitter emitter;
    bool isActive = true;
};

class VideoDecoder;

struct VideoPlayerComponent
{
    std::string filePath;
    bool isPlaying = false;
    bool isLooping = false;
    float speed = 1.0f;
    bool playOnAwake = true;

    // Runtime
    VideoDecoder* decoder = nullptr;
    bool isLoaded = false;

    void Play();
    void Pause();
    void Stop();
    void Replay();
    void Seek(double time);
};