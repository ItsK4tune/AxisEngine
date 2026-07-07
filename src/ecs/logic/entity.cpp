#include <ecs/logic/entity.h>
#include <scene/logic/scene.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/script_component.h>
#include <script/logic/input_scriptable.h>
#include <ecs/unit/terrain_component.h>
#include <navigation/unit/pathfollower_component.h>
#include <ecs/unit/network_components.h>
#include <render/logic/particle_emitter.h>
#include <resource/unit/animator.h>

bool Entity::IsValid() const
{
    return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
}

std::string Entity::GetName() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<InfoComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<InfoComponent>(m_EntityHandle).name;
    }
    return "";
}

std::string Entity::GetTag() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<InfoComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<InfoComponent>(m_EntityHandle).tag;
    }
    return "";
}

// Transform
glm::vec3 Entity::GetPosition() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<PositionComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<PositionComponent>(m_EntityHandle).value;
    }
    return glm::vec3(0.0f);
}

void Entity::SetPosition(const glm::vec3& pos)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<PositionComponent>(m_EntityHandle))
    {
        auto& p = m_Scene->GetRegistry().get<PositionComponent>(m_EntityHandle);
        p.value = p.prev = pos;
        m_Scene->SetOctreeDirty(true);
    }
}

glm::quat Entity::GetRotation() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RotationComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<RotationComponent>(m_EntityHandle).value;
    }
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void Entity::SetRotation(const glm::quat& rot)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RotationComponent>(m_EntityHandle))
    {
        auto& r = m_Scene->GetRegistry().get<RotationComponent>(m_EntityHandle);
        r.value = r.prev = rot;
        m_Scene->SetOctreeDirty(true);
    }
}

void Entity::SetRotationEuler(const glm::vec3& eulerDegrees)
{
    SetRotation(glm::quat(glm::radians(eulerDegrees)));
}

glm::vec3 Entity::GetScale() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ScaleComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<ScaleComponent>(m_EntityHandle).value;
    }
    return glm::vec3(1.0f);
}

void Entity::SetScale(const glm::vec3& scale)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ScaleComponent>(m_EntityHandle))
    {
        auto& s = m_Scene->GetRegistry().get<ScaleComponent>(m_EntityHandle);
        s.value = s.prev = scale;
        m_Scene->SetOctreeDirty(true);
    }
}

glm::mat4 Entity::GetWorldMatrix() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<WorldTransformComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<WorldTransformComponent>(m_EntityHandle).worldMatrix;
    }
    glm::mat4 t = glm::translate(glm::mat4(1.0f), GetPosition());
    glm::mat4 r = glm::toMat4(GetRotation());
    glm::mat4 s = glm::scale(glm::mat4(1.0f), GetScale());
    return t * r * s;
}

// Renderer
void Entity::SetColor(const glm::vec4& color)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).color = color;
    }
}

glm::vec4 Entity::GetColor() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).color;
    }
    return glm::vec4(1.0f);
}

void Entity::SetRenderOrder(int order)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).order = order;
    }
}

int Entity::GetRenderOrder() const
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        return m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).order;
    }
    return 0;
}

void Entity::SetCastShadow(bool castShadow)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).castShadow = castShadow;
    }
}

void Entity::SetReceiveShadow(bool receiveShadow)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).receiveShadow = receiveShadow;
    }
}

// Material
void Entity::SetMetallic(float metallic)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle).desc.pbr.metallic = metallic;
    }
}

void Entity::SetRoughness(float roughness)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle).desc.pbr.roughness = roughness;
    }
}

void Entity::SetAO(float ao)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle).desc.pbr.ao = ao;
    }
}

// Lights
void Entity::SetLightColor(const glm::vec3& color)
{
    if (IsValid())
    {
        auto& reg = m_Scene->GetRegistry();
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            reg.get<PointLightComponent>(m_EntityHandle).color = color;
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            reg.get<SpotLightComponent>(m_EntityHandle).color = color;
        else if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            reg.get<DirectionalLightComponent>(m_EntityHandle).color = color;
    }
}

void Entity::SetLightIntensity(float intensity)
{
    if (IsValid())
    {
        auto& reg = m_Scene->GetRegistry();
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            reg.get<PointLightComponent>(m_EntityHandle).intensity = intensity;
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            reg.get<SpotLightComponent>(m_EntityHandle).intensity = intensity;
        else if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            reg.get<DirectionalLightComponent>(m_EntityHandle).intensity = intensity;
    }
}

void Entity::SetLightCastShadow(bool castShadow)
{
    if (IsValid())
    {
        auto& reg = m_Scene->GetRegistry();
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            reg.get<PointLightComponent>(m_EntityHandle).isCastShadow = castShadow;
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            reg.get<SpotLightComponent>(m_EntityHandle).isCastShadow = castShadow;
        else if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            reg.get<DirectionalLightComponent>(m_EntityHandle).isCastShadow = castShadow;
    }
}

void Entity::SetLightRadius(float radius)
{
    if (IsValid())
    {
        auto& reg = m_Scene->GetRegistry();
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            reg.get<PointLightComponent>(m_EntityHandle).radius = radius;
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            reg.get<SpotLightComponent>(m_EntityHandle).radius = radius;
    }
}

// UI
void Entity::SetUIFlexAutoSize(bool autoSize)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<UIFlexLayoutComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<UIFlexLayoutComponent>(m_EntityHandle).autoSize = autoSize;
    }
}

// Animation
void Entity::PlayAnimation(const std::string& name)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<AnimationComponent>(m_EntityHandle))
    {
        auto& anim = m_Scene->GetRegistry().get<AnimationComponent>(m_EntityHandle);
        if (anim.animator)
        {
            anim.animator->PlayAnimation(name);
        }
    }
}

void Entity::SetAnimationSpeed(float speed)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<AnimationComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<AnimationComponent>(m_EntityHandle).speed = speed;
    }
}

void Entity::AddAnimationClip(const std::string& name, std::shared_ptr<Animation> animation)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<AnimationComponent>(m_EntityHandle))
    {
        auto& anim = m_Scene->GetRegistry().get<AnimationComponent>(m_EntityHandle);
        anim.animations.push_back(name);
        if (anim.animator)
        {
            anim.animator->AddAnimation(name, animation);
        }
    }
}

void Entity::CrossFade(const std::string& name, float transitionDuration)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<AnimationComponent>(m_EntityHandle))
    {
        auto& anim = m_Scene->GetRegistry().get<AnimationComponent>(m_EntityHandle);
        if (anim.animator)
        {
            anim.animator->CrossFade(name, transitionDuration);
        }
    }
}

void Entity::PlayBlend(const std::string& nameA, const std::string& nameB, float factor)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<AnimationComponent>(m_EntityHandle))
    {
        auto& anim = m_Scene->GetRegistry().get<AnimationComponent>(m_EntityHandle);
        if (anim.animator)
        {
            anim.animator->PlayBlend(nameA, nameB, factor);
        }
    }
}

// Physics
void Entity::SetLinearVelocity(const glm::vec3& velocity)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        if (rb.body)
        {
            rb.body->SetLinearVelocity(velocity);
            rb.body->Activate();
        }
    }
}

void Entity::SetAngularVelocity(const glm::vec3& velocity)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        if (rb.body)
        {
            rb.body->SetAngularVelocity(velocity);
            rb.body->Activate();
        }
    }
}

void Entity::SetKinematic(bool kinematic)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        rb.isKinematic = kinematic;
        if (rb.body)
        {
            rb.body->SetKinematic(kinematic);
            rb.body->Activate();
        }
    }
}

void Entity::SetLinearFactor(const glm::vec3& factor)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        rb.linearFactor = factor;
        if (rb.body)
        {
            rb.body->SetLinearFactor(factor);
            rb.body->Activate();
        }
    }
}

void Entity::SetAngularFactor(const glm::vec3& factor)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        rb.angularFactor = factor;
        if (rb.body)
        {
            rb.body->SetAngularFactor(factor);
            rb.body->Activate();
        }
    }
}

void Entity::AddForce(const glm::vec3& force)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        if (rb.body)
        {
            rb.body->ApplyCentralForce(force);
            rb.body->Activate();
        }
    }
}

void Entity::ApplyCentralImpulse(const glm::vec3& impulse)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        if (rb.body)
        {
            rb.body->ApplyCentralImpulse(impulse);
            rb.body->Activate();
        }
    }
}

void Entity::SetGravityEnabled(bool enabled)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<RigidBodyComponent>(m_EntityHandle))
    {
        auto& rb = m_Scene->GetRegistry().get<RigidBodyComponent>(m_EntityHandle);
        if (rb.body)
        {
            rb.body->SetLinearFactor(enabled ? glm::vec3(1.0f) : glm::vec3(1.0f, 0.0f, 1.0f));
        }
    }
}

// Reflection & Probes
void Entity::SetReflectionProbeResolution(int resolution)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ReflectionProbeComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<ReflectionProbeComponent>(m_EntityHandle).resolution = resolution;
    }
}

void Entity::SetReflectivePercent(float percent)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ReflectiveComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<ReflectiveComponent>(m_EntityHandle).reflectivity = percent;
    }
}

void Entity::SetLightProbeRadius(float radius)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<LightProbeComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<LightProbeComponent>(m_EntityHandle).radius = radius;
    }
}

void Entity::SetCameraPrimary(bool primary)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<CameraComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<CameraComponent>(m_EntityHandle).isPrimary = primary;
    }
}

void Entity::SetCameraCullingMask(uint32_t mask)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<CameraComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<CameraComponent>(m_EntityHandle).cullingMask = mask;
    }
}

void Entity::SetRenderMode(RenderMode mode)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).renderMode = mode;
    }
}

void Entity::SetIgnoreDepth(bool ignore)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MeshRendererComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<MeshRendererComponent>(m_EntityHandle).ignoreDepth = ignore;
    }
}

void Entity::SetOpacity(float opacity)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        auto& mat = m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle);
        mat.desc.opacity = opacity;
        mat.gpu.dirty = true;
    }
}

void Entity::SetBlendFactors(BlendFactor src, BlendFactor dst)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        auto& mat = m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle);
        mat.desc.blendSrc = src;
        mat.desc.blendDst = dst;
        mat.gpu.dirty = true;
    }
}

void Entity::SetReflectionProbeBox(const glm::vec3& boxMin, const glm::vec3& boxMax)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ReflectionProbeComponent>(m_EntityHandle))
    {
        auto& probe = m_Scene->GetRegistry().get<ReflectionProbeComponent>(m_EntityHandle);
        probe.boxMin = boxMin;
        probe.boxMax = boxMax;
    }
}

void Entity::SetReflectionProbeBlendDistance(float distance)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ReflectionProbeComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<ReflectionProbeComponent>(m_EntityHandle).blendDistance = distance;
    }
}

void Entity::SetReflectiveTargetProbe(const std::string& probeName)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<ReflectiveComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<ReflectiveComponent>(m_EntityHandle).targetProbe = probeName;
    }
}

void Entity::SetLightProbeIntensity(float intensity)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<LightProbeComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<LightProbeComponent>(m_EntityHandle).intensity = intensity;
    }
}

void Entity::SetLightProbeTint(const glm::vec3& tint)
{
}

void Entity::MarkTransformDirty()
{
    if (IsValid() && m_Scene->GetRegistry().all_of<WorldTransformComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<WorldTransformComponent>(m_EntityHandle).isDirty = true;
    }
}

bool Entity::GetLightCastShadow() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            return reg.get<DirectionalLightComponent>(m_EntityHandle).isCastShadow;
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            return reg.get<PointLightComponent>(m_EntityHandle).isCastShadow;
        if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            return reg.get<SpotLightComponent>(m_EntityHandle).isCastShadow;
    }
    return false;
}

float Entity::GetLightIntensity() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            return reg.get<DirectionalLightComponent>(m_EntityHandle).intensity;
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            return reg.get<PointLightComponent>(m_EntityHandle).intensity;
        if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            return reg.get<SpotLightComponent>(m_EntityHandle).intensity;
    }
    return 0.0f;
}

glm::vec3 Entity::GetLightColor() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            return reg.get<DirectionalLightComponent>(m_EntityHandle).color;
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            return reg.get<PointLightComponent>(m_EntityHandle).color;
        if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            return reg.get<SpotLightComponent>(m_EntityHandle).color;
    }
    return glm::vec3(0.0f);
}

float Entity::GetLightRadius() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            return reg.get<PointLightComponent>(m_EntityHandle).radius;
        if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            return reg.get<SpotLightComponent>(m_EntityHandle).radius;
    }
    return 0.0f;
}

glm::vec3 Entity::GetLightDirection() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            return reg.get<DirectionalLightComponent>(m_EntityHandle).direction;
    }
    return glm::vec3(0.0f, -1.0f, 0.0f);
}

void Entity::SetLightDirection(const glm::vec3& direction)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<DirectionalLightComponent>(m_EntityHandle))
    {
        m_Scene->GetRegistry().get<DirectionalLightComponent>(m_EntityHandle).direction = direction;
    }
}

void Entity::ConfigurePathFollower(bool lockXPitch, bool lockYYaw, bool lockZRoll, bool lockMoveX, bool lockMoveY, bool lockMoveZ, bool recordDebugPath, int criteria)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<PathFollowerComponent>(m_EntityHandle))
    {
        auto& pf = m_Scene->GetRegistry().get<PathFollowerComponent>(m_EntityHandle);
        pf.lockXPitch = lockXPitch;
        pf.lockYYaw = lockYYaw;
        pf.lockZRoll = lockZRoll;
        pf.lockMoveX = lockMoveX;
        pf.lockMoveY = lockMoveY;
        pf.lockMoveZ = lockMoveZ;
        pf.recordDebugPath = recordDebugPath;
        pf.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(criteria);
        pf.pathfindingOptions.preferredTags =
            criteria == 2 ? std::vector<std::string>{"road"} : std::vector<std::string>{"walkable", "road"};
        pf.pathfindingOptions.tagWeightBonus = criteria == 2 ? 80.0f : 1.0f;
        pf.pathfindingOptions.altitudePenaltyWeight = criteria == 1 ? 14.0f : 5.0f;
    }
}

void Entity::SetSpotLightCutOff(float innerDegree, float outerDegree)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<SpotLightComponent>(m_EntityHandle))
    {
        auto& spot = m_Scene->GetRegistry().get<SpotLightComponent>(m_EntityHandle);
        spot.cutOff = glm::cos(glm::radians(innerDegree));
        spot.outerCutOff = glm::cos(glm::radians(outerDegree));
    }
}

void Entity::SetLightAttenuation(float linear, float quadratic)
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
        {
            auto& light = reg.get<PointLightComponent>(m_EntityHandle);
            light.linear = linear;
            light.quadratic = quadratic;
        }
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
        {
            auto& light = reg.get<SpotLightComponent>(m_EntityHandle);
            light.linear = linear;
            light.quadratic = quadratic;
        }
    }
}

void Entity::SetEmission(const glm::vec3& emission)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<MaterialComponent>(m_EntityHandle))
    {
        auto& mat = m_Scene->GetRegistry().get<MaterialComponent>(m_EntityHandle);
        mat.desc.emission = emission;
        mat.gpu.dirty = true;
    }
}

void Entity::SetLightProbeSH(int index, const glm::vec3& coeff)
{
    if (IsValid() && m_Scene->GetRegistry().all_of<LightProbeComponent>(m_EntityHandle) && index >= 0 && index < 9)
    {
        m_Scene->GetRegistry().get<LightProbeComponent>(m_EntityHandle).sh[index] = coeff;
    }
}

void Entity::AddScriptInstance(std::unique_ptr<class IScriptable> instance, const std::string& className)
{
    if (IsValid() && instance)
    {
        auto& sc = m_Scene->GetRegistry().get_or_emplace<ScriptComponent>(m_EntityHandle);
        sc.className = className;
        sc.instance = std::move(instance);
        sc.instance->Initialize(m_EntityHandle, m_Scene);
        sc.scriptableInstance = dynamic_cast<Scriptable*>(sc.instance.get());
        sc.inputScriptableInstance = dynamic_cast<InputScriptable*>(sc.instance.get());
    }
}

void Entity::SetLightActive(bool active)
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            reg.get<DirectionalLightComponent>(m_EntityHandle).active = active;
        else if (reg.all_of<PointLightComponent>(m_EntityHandle))
            reg.get<PointLightComponent>(m_EntityHandle).active = active;
        else if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            reg.get<SpotLightComponent>(m_EntityHandle).active = active;
    }
}

bool Entity::GetLightActive() const
{
    auto& reg = m_Scene->GetRegistry();
    if (IsValid())
    {
        if (reg.all_of<DirectionalLightComponent>(m_EntityHandle))
            return reg.get<DirectionalLightComponent>(m_EntityHandle).active;
        if (reg.all_of<PointLightComponent>(m_EntityHandle))
            return reg.get<PointLightComponent>(m_EntityHandle).active;
        if (reg.all_of<SpotLightComponent>(m_EntityHandle))
            return reg.get<SpotLightComponent>(m_EntityHandle).active;
    }
    return false;
}
