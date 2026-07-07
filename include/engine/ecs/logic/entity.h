#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <render/type/graphics_types.h>
#include <render/type/render_data.h>
#include <string>
#include <memory>

struct Scene;

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene) : m_EntityHandle(handle), m_Scene(scene) {}

    bool IsValid() const;
    operator bool() const { return IsValid(); }
    operator entt::entity() const { return m_EntityHandle; }

    bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; }
    bool operator!=(const Entity& other) const { return !(*this == other); }
    bool operator==(entt::entity other) const { return m_EntityHandle == other; }
    bool operator!=(entt::entity other) const { return m_EntityHandle != other; }
    bool operator==(entt::null_t) const { return m_EntityHandle == entt::null; }
    bool operator!=(entt::null_t) const { return m_EntityHandle != entt::null; }

    entt::entity GetHandle() const { return m_EntityHandle; }
    Scene* GetScene() const { return m_Scene; }

    // Core info
    std::string GetName() const;
    std::string GetTag() const;

    // Transform
    glm::vec3 GetPosition() const;
    void SetPosition(const glm::vec3& pos);
    glm::quat GetRotation() const;
    void SetRotation(const glm::quat& rot);
    void SetRotationEuler(const glm::vec3& eulerDegrees);
    glm::vec3 GetScale() const;
    void SetScale(const glm::vec3& scale);
    glm::mat4 GetWorldMatrix() const;

    // Renderer
    void SetColor(const glm::vec4& color);
    glm::vec4 GetColor() const;
    void SetRenderOrder(int order);
    int GetRenderOrder() const;
    void SetCastShadow(bool castShadow);
    void SetReceiveShadow(bool receiveShadow);
    void SetRenderMode(RenderMode mode);
    void SetIgnoreDepth(bool ignore);

    // Material
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);
    void SetAO(float ao);
    void SetOpacity(float opacity);
    void SetBlendFactors(BlendFactor src, BlendFactor dst);
    void SetEmission(const glm::vec3& emission);

    // Lights
    void SetLightColor(const glm::vec3& color);
    void SetLightIntensity(float intensity);
    void SetLightCastShadow(bool castShadow);
    void SetLightRadius(float radius);
    void SetLightActive(bool active);
    bool GetLightActive() const;
    void SetLightDirection(const glm::vec3& direction);
    void SetSpotLightCutOff(float innerDegree, float outerDegree);
    void SetLightAttenuation(float linear, float quadratic);
    bool GetLightCastShadow() const;
    float GetLightIntensity() const;
    glm::vec3 GetLightColor() const;
    float GetLightRadius() const;
    glm::vec3 GetLightDirection() const;

    // UI
    void SetUIFlexAutoSize(bool autoSize);

    // Camera
    void SetCameraPrimary(bool primary);
    void SetCameraCullingMask(uint32_t mask);

    // Animation
    void PlayAnimation(const std::string& name);
    void SetAnimationSpeed(float speed);
    void AddAnimationClip(const std::string& name, std::shared_ptr<class Animation> animation);
    void CrossFade(const std::string& name, float transitionDuration);
    void PlayBlend(const std::string& nameA, const std::string& nameB, float factor);

    // Physics
    void SetLinearVelocity(const glm::vec3& velocity);
    void SetAngularVelocity(const glm::vec3& velocity);
    void AddForce(const glm::vec3& force);
    void ApplyCentralImpulse(const glm::vec3& impulse);
    void SetGravityEnabled(bool enabled);
    void SetKinematic(bool kinematic);
    void SetLinearFactor(const glm::vec3& factor);
    void SetAngularFactor(const glm::vec3& factor);
    void ConfigurePathFollower(bool lockXPitch, bool lockYYaw, bool lockZRoll, bool lockMoveX, bool lockMoveY, bool lockMoveZ, bool recordDebugPath, int criteria);

    // Reflection & Probes
    void SetReflectionProbeResolution(int resolution);
    void SetReflectivePercent(float percent);
    void SetLightProbeRadius(float radius);
    void SetReflectionProbeBox(const glm::vec3& boxMin, const glm::vec3& boxMax);
    void SetReflectionProbeBlendDistance(float distance);
    void SetReflectiveTargetProbe(const std::string& probeName);
    void SetLightProbeIntensity(float intensity);
    void SetLightProbeTint(const glm::vec3& tint);
    void SetLightProbeSH(int index, const glm::vec3& coeff);

    // Transform
    void MarkTransformDirty();

    // Scripts
    void AddScriptInstance(std::unique_ptr<class IScriptable> instance, const std::string& className);
    template <typename T, typename... Args>
    void AddScript(Args&&... args)
    {
        auto instance = std::make_unique<T>(std::forward<Args>(args)...);
        AddScriptInstance(std::move(instance), typeid(T).name());
    }

private:
    entt::entity m_EntityHandle = entt::null;
    Scene* m_Scene = nullptr;
};
