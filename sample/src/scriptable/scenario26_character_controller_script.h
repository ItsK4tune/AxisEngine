#pragma once
#include <axis_all.h>
#include <physics/logic/collision_matrix.h>
#include <algorithm>
#include <string>
#include <vector>

class Scenario26CharacterControllerScript : public Scriptable
{
public:
    inline static int s_JumpCount = 0;

    float moveSpeed = 6.5f;
    float sprintMultiplier = 1.75f;
    float slowMultiplier = 0.35f;
    float jumpSpeed = 11.0f;
    float stepHeight = 0.35f;
    float maxSlope = 45.0f;
    bool ignoreCharacterTrigger = false;

    int triggerEvents = 0;
    int collisionEvents = 0;
    std::string inputState = "WASD: none";
    std::string zoneState = "Zone: normal";

    void OnCreate() override
    {
        ResetRuntimeState();
    }

    void ResetCounters()
    {
        triggerEvents = 0;
        collisionEvents = 0;
        m_WasInsideTrigger = false;
        m_WasTouchingCallbackBlock = false;
    }

    void ResetRuntimeState()
    {
        ResetCounters();
        inputState = "WASD: none";
        zoneState = "Zone: normal";
        m_SlideVelocity = glm::vec3(0.0f);
        m_WasJumpHeld = false;
    }

    void OnUpdate(float dt) override
    {
        if (!HasComponent<CharacterControllerComponent>())
            return;

        auto& scene = GetScene();
        auto& cc = GetComponent<CharacterControllerComponent>();

        if (auto* collisionMatrix = Resolve<CollisionMatrix>())
        {
            collisionMatrix->Reset();
            if (ignoreCharacterTrigger)
                collisionMatrix->IgnoreTagCollision("mover", "trigger");
        }

        cc.maxSlope = maxSlope;
        if (cc.controller)
            cc.controller->SetMaxSlope(glm::radians(maxSlope));

        auto* io = Resolve<IOHandler>();
        const auto keyHeld = [io](Key key) { return io && io->GetKeyboard().GetKey(key); };

        glm::vec3 cameraForward(0.0f, 0.0f, -1.0f);
        glm::vec3 cameraRight(1.0f, 0.0f, 0.0f);
        if (auto camera = scene.GetActiveCamera(); camera != entt::null && scene.IsValid(camera))
        {
            if (auto* rot = scene.TryGetComponent<RotationComponent>(camera))
            {
                cameraForward = rot->value * glm::vec3(0.0f, 0.0f, -1.0f);
                cameraForward.y = 0.0f;
                cameraForward =
                    glm::length(cameraForward) > 0.001f ? glm::normalize(cameraForward) : glm::vec3(0.0f, 0.0f, -1.0f);

                cameraRight = rot->value * glm::vec3(1.0f, 0.0f, 0.0f);
                cameraRight.y = 0.0f;
                cameraRight = glm::length(cameraRight) > 0.001f
                                  ? glm::normalize(cameraRight)
                                  : glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));
            }
        }

        glm::vec3 move(0.0f);
        std::string nextInputState = "WASD:";
        if (GetAction("PlayerForward") || keyHeld(Key::W))
        {
            move += cameraForward;
            nextInputState += " W";
        }
        if (GetAction("PlayerBackward") || keyHeld(Key::S))
        {
            move -= cameraForward;
            nextInputState += " S";
        }
        if (GetAction("PlayerLeft") || keyHeld(Key::A))
        {
            move -= cameraRight;
            nextInputState += " A";
        }
        if (GetAction("PlayerRight") || keyHeld(Key::D))
        {
            move += cameraRight;
            nextInputState += " D";
        }

        const bool sprintHeld = keyHeld(Key::LeftShift) || keyHeld(Key::RightShift);
        const bool slowHeld = keyHeld(Key::LeftControl) || keyHeld(Key::RightControl);
        const bool jumpHeld = GetAction("PlayerJump") || keyHeld(Key::Space);
        const bool jumpPressed = jumpHeld && !m_WasJumpHeld;
        m_WasJumpHeld = jumpHeld;

        if (sprintHeld)
            nextInputState += " Shift";
        if (slowHeld)
            nextInputState += " Ctrl";
        if (jumpHeld)
            nextInputState += " Space";
        if (move.x == 0.0f && move.z == 0.0f && !jumpHeld && !sprintHeld && !slowHeld)
            nextInputState += " none";

        glm::vec3 controllerPos = glm::vec3(0.0f);
        if (cc.controller)
        {
            glm::quat controllerRot;
            cc.controller->GetWorldTransform(controllerPos, controllerRot);
        }
        else if (HasComponent<PositionComponent>())
        {
            controllerPos = GetComponent<PositionComponent>().value;
        }

        const bool insideSlowZone = PointInsideNamedBox("S26_SlowZone", controllerPos, glm::vec3(0.75f, 0.35f, 0.75f));
        const bool insideSinkZone = PointInsideNamedBox("S26_SinkZone", controllerPos, glm::vec3(0.75f, 0.35f, 0.75f));
        const bool insideFlyZone = PointInsideNamedBox("S26_FlyZone", controllerPos, glm::vec3(0.75f, 0.35f, 0.75f));
        const bool insideSlipperyZone =
            PointInsideNamedBox("S26_SlipperyZone", controllerPos, glm::vec3(0.75f, 0.35f, 0.75f));
        const bool insideBoostZone =
            PointInsideNamedBox("S26_BoostZone", controllerPos, glm::vec3(0.75f, 0.35f, 0.75f));

        float speedMultiplier = 1.0f;
        if (sprintHeld)
            speedMultiplier *= sprintMultiplier;
        if (slowHeld)
            speedMultiplier *= slowMultiplier;
        if (insideSlowZone)
            speedMultiplier *= 0.38f;
        if (insideSinkZone)
            speedMultiplier *= 0.52f;
        if (insideFlyZone)
            speedMultiplier *= 1.12f;
        if (insideBoostZone)
            speedMultiplier *= 1.85f;

        float effectiveJumpSpeed = jumpSpeed;
        float effectiveFallSpeed = 55.0f;
        glm::vec3 effectiveGravity = glm::vec3(0.0f, -29.4f, 0.0f);
        float effectiveStepHeight = stepHeight;
        glm::vec3 zoneVelocity(0.0f);
        std::string nextZoneState = "Zone:";
        bool hasZone = false;
        const auto appendZone = [&](const char* label) {
            nextZoneState += hasZone ? ", " : " ";
            nextZoneState += label;
            hasZone = true;
        };

        if (insideSlowZone)
            appendZone("slow");
        if (insideSinkZone)
        {
            appendZone("sink");
            effectiveJumpSpeed *= 0.62f;
            effectiveFallSpeed = 5.5f;
            effectiveGravity = glm::vec3(0.0f, -8.5f, 0.0f);
            effectiveStepHeight *= 0.45f;
        }
        if (insideFlyZone)
        {
            appendZone("fly");
            effectiveJumpSpeed = (std::max)(effectiveJumpSpeed, 14.0f);
            zoneVelocity.y += 5.5f;
        }
        if (insideSlipperyZone)
            appendZone("slippery");
        if (insideBoostZone)
            appendZone("boost");
        if (!hasZone)
            nextZoneState = "Zone: normal";

        cc.stepHeight = effectiveStepHeight;
        if (cc.controller)
        {
            cc.controller->SetStepHeight(effectiveStepHeight);
            cc.controller->SetFallSpeed(effectiveFallSpeed);
            cc.controller->SetGravity(effectiveGravity);
            cc.controller->SetJumpSpeed(effectiveJumpSpeed);
        }

        const float currentMoveSpeed = moveSpeed * speedMultiplier;
        const glm::vec3 desiredVelocity =
            glm::length(move) > 0.001f ? glm::normalize(move) * currentMoveSpeed : glm::vec3(0.0f);
        glm::vec3 horizontalVelocity = desiredVelocity;
        const float driveDt = glm::clamp(dt, 0.0f, 1.0f / 30.0f);
        if (insideSlipperyZone)
        {
            const float response = glm::length(move) > 0.001f ? glm::clamp(driveDt * 5.0f, 0.0f, 1.0f) : 0.0f;
            m_SlideVelocity = glm::mix(m_SlideVelocity, desiredVelocity, response);
            if (glm::length(move) <= 0.001f)
                m_SlideVelocity *= std::pow(0.18f, driveDt);
            horizontalVelocity = m_SlideVelocity;
        }
        else
        {
            m_SlideVelocity = desiredVelocity;
        }

        cc.useVelocity = true;
        cc.velocity = horizontalVelocity + zoneVelocity;
        cc.walkDirection = glm::vec3(0.0f);

        const bool onGround = cc.isOnGround || (cc.controller && cc.controller->OnGround());
        if (jumpPressed && onGround)
        {
            cc.jumpRequested = true;
            ++s_JumpCount;
        }

        inputState = nextInputState + "  speed x" + std::to_string(static_cast<int>(speedMultiplier * 100.0f)) + "%";
        zoneState = nextZoneState;

        UpdateZoneCountersAndVisuals(controllerPos, insideSlowZone, insideSinkZone, insideFlyZone, insideSlipperyZone,
                                     insideBoostZone);
    }

private:
    glm::vec3 m_SlideVelocity = glm::vec3(0.0f);
    bool m_WasJumpHeld = false;
    bool m_WasInsideTrigger = false;
    bool m_WasTouchingCallbackBlock = false;

    entt::entity FindEntityByName(const char* name)
    {
        auto view = GetScene().View<InfoComponent>();
        for (auto entity : view)
        {
            if (view.get<InfoComponent>(entity).name == name)
                return entity;
        }
        return entt::null;
    }

    bool PointInsideNamedBox(const char* name, const glm::vec3& point, const glm::vec3& padding)
    {
        auto& scene = GetScene();
        auto entity = FindEntityByName(name);
        if (entity == entt::null || !scene.IsValid(entity))
            return false;

        auto* boxPos = scene.TryGetComponent<PositionComponent>(entity);
        auto* boxScale = scene.TryGetComponent<ScaleComponent>(entity);
        if (!boxPos || !boxScale)
            return false;

        const glm::vec3 halfExtents = boxScale->value + padding;
        const glm::vec3 delta = glm::abs(point - boxPos->value);
        return delta.x <= halfExtents.x && delta.y <= halfExtents.y && delta.z <= halfExtents.z;
    }

    void SetZoneColor(const char* name, bool active, const glm::vec4& idleColor, const glm::vec4& activeColor)
    {
        auto& scene = GetScene();
        auto entity = FindEntityByName(name);
        if (entity == entt::null || !scene.IsValid(entity))
            return;

        const glm::vec4 color = active ? activeColor : idleColor;
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(entity))
            renderer->color = color;
        if (auto* mat = scene.TryGetComponent<MaterialComponent>(entity))
        {
            mat->desc.opacity = color.a;
            mat->gpu.dirty = true;
        }
    }

    void UpdateZoneCountersAndVisuals(const glm::vec3& controllerPos, bool insideSlowZone, bool insideSinkZone,
                                      bool insideFlyZone, bool insideSlipperyZone, bool insideBoostZone)
    {
        auto& scene = GetScene();
        const bool insideTrigger = PointInsideNamedBox("S26_TriggerZone", controllerPos, glm::vec3(0.75f, 0.5f, 0.75f));

        if (ignoreCharacterTrigger)
        {
            m_WasInsideTrigger = false;
        }
        else
        {
            if (insideTrigger && !m_WasInsideTrigger)
                ++triggerEvents;
            m_WasInsideTrigger = insideTrigger;
        }

        const bool touchingCallbackBlock =
            PointInsideNamedBox("S26_CallbackBlock", controllerPos, glm::vec3(0.85f, 0.6f, 0.85f));
        if (touchingCallbackBlock && !m_WasTouchingCallbackBlock)
            ++collisionEvents;
        m_WasTouchingCallbackBlock = touchingCallbackBlock;

        if (auto trigger = FindEntityByName("S26_TriggerZone"); trigger != entt::null)
        {
            glm::vec4 triggerColor = ignoreCharacterTrigger ? glm::vec4(0.95f, 0.2f, 0.15f, 0.16f)
                                     : insideTrigger        ? glm::vec4(0.15f, 1.0f, 0.42f, 0.34f)
                                                            : glm::vec4(0.1f, 0.75f, 1.0f, 0.24f);
            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(trigger))
                renderer->color = triggerColor;
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(trigger))
            {
                mat->desc.opacity = triggerColor.a;
                mat->gpu.dirty = true;
            }
        }

        SetZoneColor("S26_SlowZone", insideSlowZone, glm::vec4(0.2f, 0.28f, 1.0f, 0.22f),
                     glm::vec4(0.3f, 0.42f, 1.0f, 0.42f));
        SetZoneColor("S26_SinkZone", insideSinkZone, glm::vec4(0.18f, 0.12f, 0.05f, 0.24f),
                     glm::vec4(0.34f, 0.18f, 0.06f, 0.48f));
        SetZoneColor("S26_FlyZone", insideFlyZone, glm::vec4(0.3f, 0.95f, 1.0f, 0.2f),
                     glm::vec4(0.5f, 1.0f, 1.0f, 0.42f));
        SetZoneColor("S26_SlipperyZone", insideSlipperyZone, glm::vec4(0.72f, 0.95f, 1.0f, 0.22f),
                     glm::vec4(0.86f, 1.0f, 1.0f, 0.45f));
        SetZoneColor("S26_BoostZone", insideBoostZone, glm::vec4(1.0f, 0.72f, 0.12f, 0.22f),
                     glm::vec4(1.0f, 0.88f, 0.18f, 0.46f));

        if (auto block = FindEntityByName("S26_CallbackBlock"); block != entt::null)
        {
            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(block))
            {
                renderer->color =
                    touchingCallbackBlock ? glm::vec4(1.0f, 0.72f, 0.2f, 1.0f) : glm::vec4(0.95f, 0.55f, 0.18f, 1.0f);
            }
        }
    }
};
