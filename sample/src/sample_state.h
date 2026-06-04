#pragma once

#include <axis_all.h>
#include <physics/logic/collision_matrix.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>
#ifdef ENABLE_EDITOR
#include <imgui.h>

class ImGuiLayer;
#endif
#include <vector>

// ─── Scriptable Behaviors for Scenario 6 ───

class OrbitScript : public Scriptable
{
public:
    float speed = 1.0f;
    float radius = 5.0f;
    glm::vec3 center = glm::vec3(0.0f);
    float angle = 0.0f;

    void OnCreate() override
    {
        angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        speed = 0.5f + static_cast<float>(rand() % 100) / 100.0f;
        radius = 5.0f + static_cast<float>(rand() % 100) / 10.0f;
        if (HasComponent<PositionComponent>())
        {
            center = GetComponent<PositionComponent>().value;
        }
    }

    void OnUpdate(float dt) override
    {
        angle += speed * dt;
        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value = center + glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};

class PulseScaleScript : public Scriptable
{
public:
    float speed = 2.0f;
    float time = 0.0f;

    void OnCreate() override
    {
        speed = 1.0f + static_cast<float>(rand() % 200) / 100.0f;
        time = static_cast<float>(rand() % 100) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        time += dt;
        if (HasComponent<ScaleComponent>())
        {
            auto& scale = GetComponent<ScaleComponent>();
            float s = 1.0f + 0.4f * sin(time * speed);
            scale.value = glm::vec3(s);
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};

class ColorShiftScript : public Scriptable
{
public:
    float speed = 1.0f;
    float time = 0.0f;

    void OnCreate() override
    {
        speed = 0.5f + static_cast<float>(rand() % 100) / 100.0f;
        time = static_cast<float>(rand() % 100) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        time += dt;
        glm::vec3 color(0.5f + 0.5f * sin(time * speed), 0.5f + 0.5f * sin(time * speed + 2.0f),
                        0.5f + 0.5f * sin(time * speed + 4.0f));

        if (HasComponent<MeshRendererComponent>())
        {
            auto& renderer = GetComponent<MeshRendererComponent>();
            renderer.color = glm::vec4(color, 1.0f);
        }

        if (HasComponent<AxisMaterialComponent>())
        {
            auto& mat = GetComponent<AxisMaterialComponent>();
            mat.desc.emission = color * 0.35f;
            mat.gpu.dirty = true;
        }
    }
};

class RandomMoveScript : public Scriptable
{
public:
    float speed = 2.0f;
    float range = 10.0f;
    glm::vec3 startPos = glm::vec3(0.0f);
    glm::vec3 targetPos = glm::vec3(0.0f);
    float timer = 0.0f;
    float changeInterval = 2.0f;

    void OnCreate() override
    {
        if (HasComponent<PositionComponent>())
        {
            startPos = GetComponent<PositionComponent>().value;
            targetPos = startPos;
        }
        speed = 2.0f + static_cast<float>(rand() % 400) / 100.0f;
        changeInterval = 1.0f + static_cast<float>(rand() % 200) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        timer += dt;
        if (timer >= changeInterval)
        {
            timer = 0.0f;
            float dx = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * range;
            float dy = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * range;
            targetPos = startPos + glm::vec3(dx, dy, 0.0f);
        }

        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value = glm::mix(pos.value, targetPos, speed * dt);
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};

class RotateScript : public Scriptable
{
public:
    glm::vec3 rotationSpeed = glm::vec3(0.0f);

    void OnCreate() override
    {
        rotationSpeed = glm::vec3(static_cast<float>(rand() % 360 - 180), static_cast<float>(rand() % 360 - 180),
                                  static_cast<float>(rand() % 360 - 180));
    }

    void OnUpdate(float dt) override
    {
        if (HasComponent<RotationComponent>())
        {
            auto& rot = GetComponent<RotationComponent>();
            glm::quat deltaRot = glm::quat(glm::radians(rotationSpeed * dt));
            rot.value = rot.value * deltaRot;
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};

class BouncingScript : public Scriptable
{
public:
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 minBound = glm::vec3(-30.0f, 1.0f, -30.0f);
    glm::vec3 maxBound = glm::vec3(30.0f, 20.0f, 30.0f);

    void OnCreate() override
    {
        velocity =
            glm::normalize(glm::vec3(static_cast<float>(rand() % 200 - 100), static_cast<float>(rand() % 200 - 100),
                                     static_cast<float>(rand() % 200 - 100))) *
            (3.0f + static_cast<float>(rand() % 10));
    }

    void OnUpdate(float dt) override
    {
        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value += velocity * dt;

            if (pos.value.x < minBound.x)
            {
                pos.value.x = minBound.x;
                velocity.x *= -1.0f;
            }
            if (pos.value.x > maxBound.x)
            {
                pos.value.x = maxBound.x;
                velocity.x *= -1.0f;
            }
            if (pos.value.y < minBound.y)
            {
                pos.value.y = minBound.y;
                velocity.y *= -1.0f;
            }
            if (pos.value.y > maxBound.y)
            {
                pos.value.y = maxBound.y;
                velocity.y *= -1.0f;
            }
            if (pos.value.z < minBound.z)
            {
                pos.value.z = minBound.z;
                velocity.z *= -1.0f;
            }
            if (pos.value.z > maxBound.z)
            {
                pos.value.z = maxBound.z;
                velocity.z *= -1.0f;
            }
            if (HasComponent<WorldTransformComponent>())
                GetComponent<WorldTransformComponent>().isDirty = true;
        }
    }
};

class PlayerControlScript : public Scriptable
{
public:
    float speed = 15.0f;
    bool allowMouseColor = true;
    bool allowKeyboardWhileUI = false;
    float verticalVelocity = 0.0f;
    float groundY = 0.75f;
    bool initializedGround = false;

    void OnUpdate(float dt) override
    {
#ifdef ENABLE_EDITOR
        ImGuiIO* imguiIO = ImGui::GetCurrentContext() ? &ImGui::GetIO() : nullptr;
        const bool keyboardCaptured = imguiIO && imguiIO->WantCaptureKeyboard && !allowKeyboardWhileUI;
        const bool mouseCaptured = imguiIO && imguiIO->WantCaptureMouse;
#else
        const bool keyboardCaptured = false;
        const bool mouseCaptured = false;
#endif

        glm::vec3 move = glm::vec3(0.0f);
        if (!keyboardCaptured)
        {
            if (GetAction("PlayerForward"))
                move.z -= 1.0f;
            if (GetAction("PlayerBackward"))
                move.z += 1.0f;
            if (GetAction("PlayerLeft"))
                move.x -= 1.0f;
            if (GetAction("PlayerRight"))
                move.x += 1.0f;
        }

        bool dirty = false;
        if (glm::length(move) > 0.0f)
        {
            move = glm::normalize(move) * speed * dt;
            if (HasComponent<PositionComponent>())
            {
                auto& pos = GetComponent<PositionComponent>();
                pos.value += move;
                dirty = true;
            }
        }

        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            if (!initializedGround)
            {
                groundY = pos.value.y;
                initializedGround = true;
            }

            const bool onGround = pos.value.y <= groundY + 0.001f;
            if (!keyboardCaptured && GetActionDown("PlayerJump") && onGround)
            {
                verticalVelocity = 11.0f;
            }

            verticalVelocity -= 28.0f * dt;
            pos.value.y += verticalVelocity * dt;
            if (pos.value.y < groundY)
            {
                pos.value.y = groundY;
                verticalVelocity = 0.0f;
            }
            dirty = true;
        }

        if (allowMouseColor && !mouseCaptured && GetActionDown("PlayerAction"))
        {
            if (HasComponent<AxisMaterialComponent>())
            {
                auto& mat = GetComponent<AxisMaterialComponent>();
                mat.desc.pbr.roughness = static_cast<float>(rand() % 100) / 100.0f;
                mat.desc.pbr.metallic = static_cast<float>(rand() % 100) / 100.0f;
                mat.desc.emission =
                    glm::vec3(static_cast<float>(rand() % 100) / 30.0f, static_cast<float>(rand() % 100) / 30.0f,
                              static_cast<float>(rand() % 100) / 30.0f);
                mat.gpu.dirty = true;
            }
        }

        if (HasComponent<ScaleComponent>())
        {
            auto& scale = GetComponent<ScaleComponent>();
            scale.value = glm::mix(scale.value, glm::vec3(1.5f), 10.0f * dt);
            dirty = true;
        }

        if (dirty && HasComponent<WorldTransformComponent>())
            GetComponent<WorldTransformComponent>().isDirty = true;
    }
};

class CharacterControllerDemoScript : public Scriptable
{
public:
    inline static int s_JumpCount = 0;
    float speed = 8.0f;

    void OnUpdate(float dt) override
    {
        (void)dt;
        if (!HasComponent<CharacterControllerComponent>())
            return;

        auto& cc = GetComponent<CharacterControllerComponent>();
        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        const auto keyHeld = [io](Key key) { return io && io->GetKeyboard().GetKey(key); };
        const auto keyDown = [io](Key key) { return io && io->GetKeyboard().IsKeyDown(key); };

        glm::vec3 move(0.0f);
        if (GetAction("PlayerForward") || keyHeld(Key::W))
            move.z -= 1.0f;
        if (GetAction("PlayerBackward") || keyHeld(Key::S))
            move.z += 1.0f;
        if (GetAction("PlayerLeft") || keyHeld(Key::A))
            move.x -= 1.0f;
        if (GetAction("PlayerRight") || keyHeld(Key::D))
            move.x += 1.0f;

        const bool sprintHeld = keyHeld(Key::LeftShift) || keyHeld(Key::RightShift);
        const bool slowHeld = keyHeld(Key::LeftControl) || keyHeld(Key::RightControl);
        float currentSpeed = speed * (sprintHeld ? 1.75f : 1.0f);
        if (slowHeld)
            currentSpeed *= 0.35f;
        cc.useVelocity = true;
        cc.velocity = glm::length(move) > 0.001f ? glm::normalize(move) * currentSpeed : glm::vec3(0.0f);
        cc.walkDirection = glm::vec3(0.0f);

        if ((GetActionDown("PlayerJump") || keyDown(Key::Space)) && cc.isOnGround)
        {
            cc.jumpRequested = true;
            ++s_JumpCount;
        }
    }
};

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

        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        const auto keyHeld = [io](Key key) { return io && io->GetKeyboard().GetKey(key); };

        glm::vec3 cameraForward(0.0f, 0.0f, -1.0f);
        glm::vec3 cameraRight(1.0f, 0.0f, 0.0f);
        if (auto camera = EntityManager::GetActiveCamera(scene); camera != entt::null && scene.registry.valid(camera))
        {
            if (auto* rot = scene.registry.try_get<RotationComponent>(camera))
            {
                cameraForward = rot->value * glm::vec3(0.0f, 0.0f, -1.0f);
                cameraForward.y = 0.0f;
                cameraForward = glm::length(cameraForward) > 0.001f ? glm::normalize(cameraForward)
                                                                     : glm::vec3(0.0f, 0.0f, -1.0f);

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
        auto view = GetScene().registry.view<InfoComponent>();
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
        if (entity == entt::null || !scene.registry.valid(entity))
            return false;

        auto* boxPos = scene.registry.try_get<PositionComponent>(entity);
        auto* boxScale = scene.registry.try_get<ScaleComponent>(entity);
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
        if (entity == entt::null || !scene.registry.valid(entity))
            return;

        const glm::vec4 color = active ? activeColor : idleColor;
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            renderer->color = color;
        if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(entity))
        {
            mat->desc.opacity = color.a;
            mat->gpu.dirty = true;
        }
    }

    void UpdateZoneCountersAndVisuals(const glm::vec3& controllerPos, bool insideSlowZone, bool insideSinkZone,
                                      bool insideFlyZone, bool insideSlipperyZone, bool insideBoostZone)
    {
        auto& scene = GetScene();
        const bool insideTrigger =
            PointInsideNamedBox("S26_TriggerZone", controllerPos, glm::vec3(0.75f, 0.5f, 0.75f));

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
            if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(trigger))
                renderer->color = triggerColor;
            if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(trigger))
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
            if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(block))
            {
                renderer->color = touchingCallbackBlock ? glm::vec4(1.0f, 0.72f, 0.2f, 1.0f)
                                                        : glm::vec4(0.95f, 0.55f, 0.18f, 1.0f);
            }
        }
    }
};

class Scenario26FpsCameraScript : public Scriptable
{
public:
    inline static float s_Yaw = -90.0f;
    inline static float s_Pitch = -8.0f;

    void OnCreate() override
    {
        s_Yaw = -90.0f;
        s_Pitch = -8.0f;
    }

    void OnUpdate(float dt) override
    {
        (void)dt;
        if (!HasComponent<PositionComponent>() || !HasComponent<RotationComponent>())
            return;

        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        if (!io)
            return;

        if (m_Target == entt::null || !GetScene().registry.valid(m_Target))
        {
            auto view = GetScene().registry.view<InfoComponent>();
            for (auto entity : view)
            {
                if (view.get<InfoComponent>(entity).name == "S26_CharacterController")
                {
                    m_Target = entity;
                    break;
                }
            }
        }

        if (m_Target == entt::null || !GetScene().registry.valid(m_Target))
            return;

        bool mouseCaptured = false;
#ifdef ENABLE_EDITOR
        if (ImGui::GetCurrentContext())
            mouseCaptured = ImGui::GetIO().WantCaptureMouse;
#endif

        auto& mouse = io->GetMouse();
        const bool looking = mouse.IsRightButtonPressed() && !mouseCaptured;
        if (looking)
        {
            if (mouse.GetCursorMode() != CursorMode::LockedHidden)
                mouse.SetCursorMode(CursorMode::LockedHidden);
            s_Yaw += mouse.GetXOffset() * m_MouseSensitivity;
            s_Pitch += mouse.GetYOffset() * m_MouseSensitivity;
            s_Pitch = glm::clamp(s_Pitch, -82.0f, 82.0f);
        }
        else if (mouse.GetCursorMode() == CursorMode::LockedHidden)
        {
            mouse.SetCursorMode(CursorMode::Normal);
        }

        auto& targetPos = GetScene().registry.get<PositionComponent>(m_Target);
        glm::vec3 front;
        front.x = std::cos(glm::radians(s_Yaw)) * std::cos(glm::radians(s_Pitch));
        front.y = std::sin(glm::radians(s_Pitch));
        front.z = std::sin(glm::radians(s_Yaw)) * std::cos(glm::radians(s_Pitch));
        front = glm::normalize(front);

        GetComponent<PositionComponent>().value = targetPos.value + glm::vec3(0.0f, 1.45f, 0.0f);
        GetComponent<RotationComponent>().value = glm::quatLookAt(front, glm::vec3(0.0f, 1.0f, 0.0f));
        if (HasComponent<WorldTransformComponent>())
            GetComponent<WorldTransformComponent>().isDirty = true;
    }

private:
    entt::entity m_Target = entt::null;
    float m_MouseSensitivity = 0.1f;
};

class CollisionReporterScript : public Scriptable
{
public:
    inline static int s_CollisionEnterCount = 0;
    inline static int s_TriggerEnterCount = 0;

    void OnCollisionEnter(entt::entity other) override
    {
        (void)other;
        ++s_CollisionEnterCount;
        Mark(glm::vec4(1.0f, 0.55f, 0.12f, 1.0f), glm::vec3(0.8f, 0.25f, 0.05f));
    }

    void OnTriggerEnter(entt::entity other) override
    {
        (void)other;
        ++s_TriggerEnterCount;
        Mark(glm::vec4(0.2f, 0.95f, 1.0f, 1.0f), glm::vec3(0.0f, 0.7f, 0.9f));
    }

private:
    void Mark(const glm::vec4& color, const glm::vec3& emission)
    {
        if (HasComponent<MeshRendererComponent>())
            GetComponent<MeshRendererComponent>().color = color;
        if (HasComponent<AxisMaterialComponent>())
        {
            auto& mat = GetComponent<AxisMaterialComponent>();
            mat.desc.emission = emission;
            mat.gpu.dirty = true;
        }
    }
};

// REGISTER macros for static factory registrations
REGISTER_SCRIPT(OrbitScript)
REGISTER_SCRIPT(PulseScaleScript)
REGISTER_SCRIPT(ColorShiftScript)
REGISTER_SCRIPT(RandomMoveScript)
REGISTER_SCRIPT(RotateScript)
REGISTER_SCRIPT(BouncingScript)
REGISTER_SCRIPT(PlayerControlScript)
REGISTER_SCRIPT(CharacterControllerDemoScript)
REGISTER_SCRIPT(Scenario26CharacterControllerScript)
REGISTER_SCRIPT(Scenario26FpsCameraScript)
REGISTER_SCRIPT(CollisionReporterScript)

// ─── SampleState Declaration ───

class SampleState : public State
{
public:
    SampleState() = default;
    ~SampleState() override = default;

    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnRenderDebug() override;
    void OnExit() override;

private:
    void LoadScenario(int index);
    void ResetDefaultPlayerBindings();
    void ApplyScenario7RenderOrder();
    void DrawGUI();
    void SetupCamera();

    void LoadScene1();   // Render Entity Load
    void LoadScene2();   // Lighting Load Test
    void LoadScene3();   // Shadow Mapping Test
    void LoadScene4();   // Shadow Receiver
    void LoadScene5();   // PBR Material Matrix
    void LoadScene6();   // Transparent Objects
    void LoadScene7();   // Render Order
    void LoadScene8();   // Layer Filter
    void LoadScene9();   // LOD Selection
    void LoadScene10();  // Post-Process & Tonemap
    void LoadScene11();  // SSR & Env Probes
    void LoadScene12();  // Light Probe
    void LoadScene13();  // Decal Stress Test
    void LoadScene14();  // Terrain Creation Showcase
    void LoadScene15();  // Particle Stress Test
    void LoadScene16();  // UI & Responsive Showcase
    void LoadScene17();  // Interactive UI
    void LoadScene18();  // Video Mesh & UI Render
    void LoadScene19();  // Skeletal Anim & Blend
    void LoadScene20();  // Physics Stress Test
    void LoadScene21();  // Physics Constraint Chain
    void LoadScene22();  // Hitray & Hitscan
    void LoadScene23();  // Navigation Test
    void LoadScene24();  // Scriptable Stability Test
    void LoadScene25();  // Scene Save & Load
    void LoadScene26();  // Character Controller + Collision Zone
    void LoadScene27();  // DataNote YAML Test
    void LoadScene28();  // Input Binding Save/Load
    void LoadScene29();  // Localization
    void LoadScene30();  // Network Messaging
    void LoadScene31();  // Audio 2D & 3D
    void StopScenario31Audio();

#ifdef ENABLE_EDITOR
    ImGuiLayer* m_EditorImGuiLayer = nullptr;
    bool m_EditorSystemEnabled = false;
#endif
    int m_CurrentScenario = 0;
    int m_PendingScenario = -1;  // scenario to load next OnUpdate
    bool m_ShowDebugLines = true;

    // Scenario 3 parameters
    int m_S3LightMotionMode = 1;  // 0: Static, 1: Circle, 2: Vertical bob, 3: Figure eight
    float m_S3PointMotionSpeed = 1.0f;
    float m_S3PointOrbitRadius = 18.0f;
    float m_S3PointMotionHeight = 12.0f;
    float m_S3SpotMotionSpeed = 0.75f;
    float m_S3SpotOrbitRadius = 24.0f;
    float m_S3SpotMotionHeight = 22.0f;
    float m_S3DirectionalSweepSpeed = 0.25f;
    float m_S3MotionTime = 0.0f;
    glm::vec3 m_S3DirectionalColor = glm::vec3(1.0f, 0.95f, 0.9f);
    glm::vec3 m_S3PointColor = glm::vec3(1.0f, 0.2f, 0.2f);
    glm::vec3 m_S3SpotColor = glm::vec3(0.2f, 0.2f, 1.0f);
    float m_S3DirectionalIntensity = 1.5f;
    float m_S3PointIntensity = 5.0f;
    float m_S3SpotIntensity = 8.0f;

    // Scenario 2 parameters
    float m_S2DirectionalIntensity = 0.08f;
    float m_S2PointIntensity = 2.0f;
    float m_S2SpotIntensity = 3.0f;
    glm::vec3 m_S2DirectionalColor = glm::vec3(1.0f);
    glm::vec3 m_S2PointColor = glm::vec3(1.0f);
    glm::vec3 m_S2SpotColor = glm::vec3(1.0f);

    // Scenario 1 parameters
    int m_S1EntityCount = 10000;
    int m_S1MeshType = 0;  // 0: Sphere, 1: Cube, 2: Cylinder, 3: Capsule
    bool m_S1UniqueTint = false;

    // Scenario 20 parameters
    int m_S20EntityCount = 100;  // Default lower to be safe
    int m_S20ShapeType = 0;      // 0: Box, 1: Sphere, 2: Capsule
    float m_S20Mass = 1.0f;
    float m_S20Restitution = 0.5f;
    float m_S20Friction = 0.5f;
    glm::vec3 m_S20Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float m_S20SpawnHeight = 12.0f;
    float m_S20GridSpacing = 2.5f;
    float m_S20LinearDamping = 0.04f;
    float m_S20AngularDamping = 0.12f;
    float m_S20InitialImpulse = 0.0f;

    // Scenario 23 parameters
    int m_S23ObstacleCount = 12;
    float m_S23ObstacleSize = 5.0f;
    float m_S23FollowerSpeed = 8.0f;
    bool m_S23LockXPitch = false;
    bool m_S23LockYYaw = false;
    bool m_S23LockZRoll = false;
    bool m_S23LockMoveX = false;
    bool m_S23LockMoveY = false;
    bool m_S23LockMoveZ = false;
    int m_S23PathfindingCriteria = 0;  // See PathfindingCriteria combo in DrawGUI.
    glm::vec3 m_S23NewWaypoint = glm::vec3(0.0f, 0.5f, 0.0f);
    bool m_S23RepathRequested = false;
    int m_S23LastPathfindingCriteria = -1;

    // Scenario 24 parameters
    int m_S24EntityCount = 100;
    int m_S24MeshType = 0;    // 0: Cube, 1: Sphere, 2: Capsule, 3: Cylinder
    int m_S24ShaderMode = 0;  // 0: Deferred unlit, 1: Deferred lit, 2: Forward lit
    float m_S24BaseRadius = 10.0f;
    float m_S24RadiusStep = 3.0f;
    float m_S24VerticalStep = 2.0f;
    float m_S24EntityScale = 1.0f;
    bool m_S24EnableOrbit = true;
    bool m_S24EnablePulse = true;
    bool m_S24EnableColor = true;
    bool m_S24EnableRandomMove = true;
    bool m_S24EnableRotate = true;
    bool m_S24EnableBounce = true;

    // Scenario 15 parameters
    int m_S15EmitterCount = 50;
    float m_S15SpawnRate = 200.0f;
    float m_S15LifeTime = 2.0f;
    float m_S15StartSize = 0.5f;
    float m_S15EndSize = 0.05f;
    float m_S15MinSpeed = 2.0f;
    float m_S15MaxSpeed = 4.0f;
    float m_S15VerticalSpeed = 2.0f;

    // Scenario 21 parameters
    int m_S21ChainLength = 6;
    float m_S21WindForce = 0.0f;
    glm::vec3 m_S21Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float m_S21LinkMass = 1.5f;
    float m_S21PayloadMass = 4.0f;
    float m_S21LinkDamping = 0.3f;
    float m_S21AnchorHeight = 22.0f;
    float m_S21KickForce = 35.0f;
    int m_S21LinkShape = 0;
    int m_S21PayloadShape = 1;
    std::vector<entt::entity> m_S21ChainEntities;

    // Scenario 13 parameters
    int m_S13DecalCount = 15;
    float m_S13DecalSize = 3.0f;
    float m_S13Opacity = 1.0f;
    glm::vec3 m_S13Color = glm::vec3(1.0f, 0.2f, 0.2f);
    glm::vec3 m_S13LightColor = glm::vec3(1.0f);
    float m_S13LightIntensity = 1.5f;
    int m_S13LightingMode = 2;  // 0: Unlit, 1: Lit, 2: Lit + shadow
    bool m_S13UsePointLight = true;
    bool m_S13ShowShadowCaster = true;
    glm::vec3 m_S13PointLightColor = glm::vec3(1.0f, 0.85f, 0.45f);
    float m_S13PointLightIntensity = 5.0f;
    float m_S13PointLightRadius = 35.0f;
    bool m_S13RainbowMode = false;

    // Scenario 25 UI variables
    std::string m_S25Status = "Ready";
    int m_S25RandomEntityCount = 0;
    std::vector<entt::entity> m_S25RandomEntities;

    // Scenario 28 UI variables
    std::string m_S28Status = "Ready";
    std::string m_S28NewAction = "PlayerJump";
    int m_S28NewKey = 32;  // Space

    // Scenario 27 UI variables
    std::string m_S27Status = "Ready";
    int m_S27Level = 42;
    int m_S27XP = 12500;
    int m_S27EntityCount = 24;
    float m_S27EntitySize = 2.0f;

    // Scenario 30 UI variables
    std::string m_S30Status = "Stopped";
    bool m_S30IsServer = true;
    std::vector<std::string> m_S30Messages;
    float m_S30SendTimer = 0.0f;
    int m_S30SpawnCounter = 0;
    char m_S30MessageText[128] = "Hello from AxisEngine";
    char m_S30Host[64] = "127.0.0.1";
    int m_S30Port = 12345;
    bool m_S30UseIPv6 = false;

    // Scenario 31 UI variables
    std::shared_ptr<ISound> m_S31Audio2D = nullptr;
    std::shared_ptr<ISound> m_S31Audio3D = nullptr;
    float m_S31OrbitAngle = 0.0f;
    float m_S31Speed = 1.0f;
    float m_S31Volume2D = 0.35f;
    float m_S31Volume3D = 0.8f;
    float m_S31Pitch = 1.0f;
    float m_S31MinDistance = 2.0f;
    float m_S31MaxDistance = 50.0f;
    bool m_S31Play2D = false;
    bool m_S31Play3D = false;

    // Scenario 18 UI variables
    bool m_S18VideoPlaying = true;
    float m_S18Volume = 1.0f;

    // Scenario 19 UI variables
    int m_S19AnimIndex = 0;
    float m_S19Blend = 0.5f;
    float m_S19Speed = 1.0f;

    // Scenario 11 UI variables
    int m_S11ProbeResolution = 512;
    float m_S11Reflectivity = 0.65f;
    float m_S11FresnelBias = 0.04f;
    float m_S11FresnelPower = 5.0f;
    int m_S11ActiveCase = 0;
    std::vector<entt::entity> m_S11ReflectionSpheres;
    std::vector<entt::entity> m_S11ReflectionProbes;
    entt::entity m_S11PlanarMirror = entt::null;

    // Scenario 6 UI variables
    float m_S6GlassOpacity = 0.35f;
    float m_S6GlassRoughness = 0.12f;
    bool m_S6AnimateObjects = true;

    // Scenario 7/8 UI variables
    int m_S8LayerMask = 0x7;
    bool m_S7ReverseOrder = false;

    // Scenario 4 UI variables
    float m_S4ReceiverSize = 85.0f;
    float m_S4CasterHeight = 5.0f;
    float m_S4CasterScale = 1.0f;
    float m_S4CasterSpread = 1.0f;
    float m_S4LightYaw = -40.0f;
    float m_S4LightPitch = -50.0f;
    float m_S4LightIntensity = 1.4f;
    bool m_S4AnimateCasters = false;
    float m_S4AnimTime = 0.0f;

    // Scenario 16 UI variables
    float m_S16RotateCard = 12.0f;
    bool m_S16ShowTexture = true;
    bool m_S16FlipTextureX = false;
    bool m_S16FlipTextureY = false;
    int m_S16LayoutMode = 0;
    float m_S16PanelAlpha = 0.92f;

    // Scenario 14 Terrain variables
    float m_S14TerrainWidth = 200.0f;
    float m_S14TerrainHeight = 35.0f;
    float m_S14TerrainLength = 200.0f;
    float m_S14TextureScale = 12.0f;
    bool m_S14GeneratePhysics = true;
    bool m_S14SpawnPhysicsBalls = false;
    float m_S14NoiseFrequency = 1.8f;
    int m_S14NoiseOctaves = 4;
    float m_S14SpawnTimer = 0.0f;

    // Scenario 22 ray query variables
    float m_S22Yaw = -90.0f;
    float m_S22Pitch = -4.0f;
    float m_S22Distance = 90.0f;
    float m_S22ImpactImpulse = 16.0f;
    bool m_S22AutoSweep = true;
    bool m_S22FireRequested = false;
    bool m_S22RayHit = false;
    float m_S22SweepTime = 0.0f;
    float m_S22ShotFlash = 0.0f;
    int m_S22HitCount = 0;
    std::string m_S22LastHit = "No hit";
    glm::vec3 m_S22RayOrigin = glm::vec3(0.0f);
    glm::vec3 m_S22RayEnd = glm::vec3(0.0f);
    entt::entity m_S22EmitterEntity = entt::null;
    entt::entity m_S22LastHitEntity = entt::null;
    std::vector<entt::entity> m_S22Targets;

    // Scenario 12/26 variables
    float m_S12ProbeIntensity = 1.0f;
    float m_S12ProbeRadius = 14.0f;
    float m_S26MoveSpeed = 6.5f;
    float m_S26SprintMultiplier = 1.75f;
    float m_S26SlowMultiplier = 0.35f;
    float m_S26JumpSpeed = 11.0f;
    float m_S26StepHeight = 0.35f;
    float m_S26MaxSlope = 45.0f;
    bool m_S26IgnoreCharacterTrigger = false;
    entt::entity m_S26ControllerEntity = entt::null;

    // Post-processing UI state
    bool m_PPHdrEnabled = true;
    bool m_PPBloomEnabled = true;
    float m_PPBloomThreshold = 0.8f;
    float m_PPBloomIntensity = 1.5f;
    float m_PPBloomRadius = 0.005f;
    float m_PPExposure = 1.0f;
    float m_PPGamma = 2.2f;
    int m_PPTonemappingMode = 1;
    bool m_PPVignetteEnabled = false;
    bool m_PPGlitchEnabled = false;
    bool m_PPFilmGrainEnabled = false;
    bool m_PPGrayEnabled = false;
    bool m_PPDitherEnabled = false;
    bool m_PPPartialEffectEnabled = false;
    int m_PPPartialEffectType = 0;
    int m_PPPartialX = 0;
    int m_PPPartialY = 0;
    int m_PPPartialW = 0;
    int m_PPPartialH = 0;

    // Benchmarking counters
    float m_FpsTime = 0.0f;
    int m_FpsCount = 0;
    float m_CurrentFps = 0.0f;

    // Scenario 5 navigation state
    entt::entity m_NavFollower = entt::null;
    std::vector<glm::vec3> m_NavWaypoints;
    int m_CurrentWaypointIndex = 0;

    // Character controller playground drag state
    entt::entity m_S24GrabbedEntity = entt::null;
    glm::vec3 m_S24GrabOffset = glm::vec3(0.0f);
    float m_S24GrabPlaneY = 0.0f;
    bool m_S24Dragging = false;

    // Cached entity handles (set in LoadScene*, used in OnUpdate to avoid per-frame O(n) scans)
    entt::entity m_S3DirLightEntity = entt::null;
    entt::entity m_S3PointLightEntity = entt::null;
    entt::entity m_S3SpotLightEntity = entt::null;
    entt::entity m_S13DirLightEntity = entt::null;
    entt::entity m_S13PointLightEntity = entt::null;
    entt::entity m_S13PointLightMarkerEntity = entt::null;
    entt::entity m_S13ShadowCasterEntity = entt::null;
    entt::entity m_S4ReceiverEntity = entt::null;
    entt::entity m_S4DeferredCubeEntity = entt::null;
    entt::entity m_S4DeferredSphereEntity = entt::null;
    entt::entity m_S4ForwardCubeEntity = entt::null;
    entt::entity m_S4ForwardCapsuleEntity = entt::null;
    entt::entity m_S4LightEntity = entt::null;
    entt::entity m_S16CardEntity = entt::null;
    entt::entity m_S16TextureEntity = entt::null;
    entt::entity m_S16RootPanel = entt::null;
};
