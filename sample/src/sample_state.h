#pragma once

#include <axis_all.h>
#include <imgui.h>
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
        glm::vec3 color(
            0.5f + 0.5f * sin(time * speed),
            0.5f + 0.5f * sin(time * speed + 2.0f),
            0.5f + 0.5f * sin(time * speed + 4.0f)
        );

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
        rotationSpeed = glm::vec3(
            static_cast<float>(rand() % 360 - 180),
            static_cast<float>(rand() % 360 - 180),
            static_cast<float>(rand() % 360 - 180)
        );
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
        velocity = glm::normalize(glm::vec3(
            static_cast<float>(rand() % 200 - 100),
            static_cast<float>(rand() % 200 - 100),
            static_cast<float>(rand() % 200 - 100)
        )) * (3.0f + static_cast<float>(rand() % 10));
    }

    void OnUpdate(float dt) override
    {
        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            pos.value += velocity * dt;

            if (pos.value.x < minBound.x) { pos.value.x = minBound.x; velocity.x *= -1.0f; }
            if (pos.value.x > maxBound.x) { pos.value.x = maxBound.x; velocity.x *= -1.0f; }
            if (pos.value.y < minBound.y) { pos.value.y = minBound.y; velocity.y *= -1.0f; }
            if (pos.value.y > maxBound.y) { pos.value.y = maxBound.y; velocity.y *= -1.0f; }
            if (pos.value.z < minBound.z) { pos.value.z = minBound.z; velocity.z *= -1.0f; }
            if (pos.value.z > maxBound.z) { pos.value.z = maxBound.z; velocity.z *= -1.0f; }
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
    float verticalVelocity = 0.0f;
    float groundY = 0.75f;
    bool initializedGround = false;

    void OnUpdate(float dt) override
    {
        ImGuiIO* imguiIO = ImGui::GetCurrentContext() ? &ImGui::GetIO() : nullptr;
        const bool keyboardCaptured = imguiIO && imguiIO->WantCaptureKeyboard;
        const bool mouseCaptured = imguiIO && imguiIO->WantCaptureMouse;

        glm::vec3 move = glm::vec3(0.0f);
        if (!keyboardCaptured)
        {
            if (GetAction("PlayerForward")) move.z -= 1.0f;
            if (GetAction("PlayerBackward")) move.z += 1.0f;
            if (GetAction("PlayerLeft")) move.x -= 1.0f;
            if (GetAction("PlayerRight")) move.x += 1.0f;
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
                mat.desc.emission = glm::vec3(
                    static_cast<float>(rand() % 100) / 30.0f,
                    static_cast<float>(rand() % 100) / 30.0f,
                    static_cast<float>(rand() % 100) / 30.0f
                );
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

// REGISTER macros for static factory registrations
REGISTER_SCRIPT(OrbitScript)
REGISTER_SCRIPT(PulseScaleScript)
REGISTER_SCRIPT(ColorShiftScript)
REGISTER_SCRIPT(RandomMoveScript)
REGISTER_SCRIPT(RotateScript)
REGISTER_SCRIPT(BouncingScript)
REGISTER_SCRIPT(PlayerControlScript)

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
    void DrawGUI();
    void SetupCamera();

    void LoadScene1(); // 10,000 non-colliding spheres
    void LoadScene2(); // 1,000 cubes, plane, 3 light types with shadow mapping
    void LoadScene3(); // 1 cylinder, plane, 999 lighting load (333 dir, 333 point, 333 spot)
    void LoadScene4(); // 1,000 dynamic rigidbodies falling
    void LoadScene5(); // Navmesh generation + follower pathfinding
    void LoadScene6(); // Scriptable stability (100 entities with 6 script behaviors)
    void LoadScene7(); // Particle emitter stress test (dynamic colored vortex)
    void LoadScene8(); // Scenario 8: Interactive Playground (Mouse & Keyboard)
    void LoadScene9(); // Scenario 9: Post-Processing & Tonemapping
    void LoadScene10(); // Scenario 10: Physics Constraint Chain (Pendulum)
    void LoadScene11(); // Scenario 11: Decal Stress Test & Blending
    void LoadScene12(); // Scenario 12: Scene Save & Load Test
    void LoadScene13(); // Scenario 13: Input Binding Save & Load Test
    void LoadScene14(); // Scenario 14: Localization (l10n) Test
    void LoadScene15(); // Scenario 15: AxisData Node Save & Load Test
    void LoadScene16(); // Scenario 16: Network Messaging Test
    void LoadScene17(); // Scenario 17: Audio 2D/3D Test
    void LoadScene18(); // Scenario 18: Video Playback Test
    void LoadScene19(); // Scenario 19: Skeletal Animation Test
    void LoadScene20(); // Scenario 20: Reflection & Environment Probes Test
    void LoadScene21(); // Scenario 21: Transparent Object Sorting Test
    void LoadScene22(); // Scenario 22: PBR Material Matrix Test
    void LoadScene23(); // Scenario 23: LOD Selection Test
    void LoadScene24(); // Scenario 24: Camera Layer Filter Test
    void LoadScene25(); // Scenario 25: Render Order Test
    void LoadScene26(); // Scenario 26: Instanced Batching Test
    void LoadScene27(); // Scenario 27: Deferred Shadow Receiver Test
    void LoadScene28(); // Scenario 28: UI Showcase
    void LoadScene29(); // Scenario 29: Responsive UI

    ImGuiLayer* m_EditorImGuiLayer = nullptr;
    int m_CurrentScenario = 0;
    int m_PendingScenario = -1;  // scenario to load next OnUpdate
    bool m_ShowDebugLines = true;
    bool m_EditorSystemEnabled = false;

    // Scenario 2 parameters
    int m_S2LightMotionMode = 1; // 0: Static, 1: Circle, 2: Vertical bob, 3: Figure eight
    float m_S2PointMotionSpeed = 1.0f;
    float m_S2PointOrbitRadius = 18.0f;
    float m_S2PointMotionHeight = 12.0f;
    float m_S2SpotMotionSpeed = 0.75f;
    float m_S2SpotOrbitRadius = 24.0f;
    float m_S2SpotMotionHeight = 22.0f;
    float m_S2DirectionalSweepSpeed = 0.25f;
    float m_S2MotionTime = 0.0f;
    glm::vec3 m_S2DirectionalColor = glm::vec3(1.0f, 0.95f, 0.9f);
    glm::vec3 m_S2PointColor = glm::vec3(1.0f, 0.2f, 0.2f);
    glm::vec3 m_S2SpotColor = glm::vec3(0.2f, 0.2f, 1.0f);
    float m_S2DirectionalIntensity = 1.5f;
    float m_S2PointIntensity = 5.0f;
    float m_S2SpotIntensity = 8.0f;

    // Scenario 3 parameters
    float m_S3DirectionalIntensity = 0.02f;
    float m_S3PointIntensity = 2.0f;
    float m_S3SpotIntensity = 3.0f;
    glm::vec3 m_S3DirectionalColor = glm::vec3(1.0f);
    glm::vec3 m_S3PointColor = glm::vec3(1.0f);
    glm::vec3 m_S3SpotColor = glm::vec3(1.0f);

    // Scenario 1 parameters
    int m_S1EntityCount = 10000;
    int m_S1MeshType = 0; // 0: Sphere, 1: Cube, 2: Cylinder, 3: Capsule

    // Scenario 4 parameters
    int m_S4EntityCount = 100; // Default lower to be safe
    int m_S4ShapeType = 0; // 0: Box, 1: Sphere, 2: Capsule
    float m_S4Mass = 1.0f;
    float m_S4Restitution = 0.5f;
    float m_S4Friction = 0.5f;
    glm::vec3 m_S4Gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    // Scenario 5 parameters
    int m_S5ObstacleCount = 12;
    float m_S5ObstacleSize = 5.0f;
    float m_S5FollowerSpeed = 8.0f;
    bool m_S5LockXPitch = false;
    bool m_S5LockYYaw = false;
    bool m_S5LockZRoll = false;
    int m_S5PathfindingCriteria = 0; // 0: Shortest, 1: Smoothest, 2: StayOnRoad
    glm::vec3 m_S5NewWaypoint = glm::vec3(0.0f, 0.5f, 0.0f);

    // Scenario 7 parameters
    int m_S7EmitterCount = 50;
    float m_S7SpawnRate = 200.0f;
    float m_S7LifeTime = 2.0f;
    float m_S7StartSize = 0.5f;
    float m_S7EndSize = 0.05f;
    float m_S7MinSpeed = 2.0f;
    float m_S7MaxSpeed = 4.0f;
    float m_S7VerticalSpeed = 2.0f;

    // Scenario 10 parameters
    int m_S10ChainLength = 6;
    float m_S10WindForce = 0.0f;
    glm::vec3 m_S10Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    std::vector<entt::entity> m_S10ChainEntities;

    // Scenario 11 parameters
    int m_S11DecalCount = 15;
    float m_S11DecalSize = 3.0f;
    float m_S11Opacity = 1.0f;
    glm::vec3 m_S11Color = glm::vec3(1.0f, 0.2f, 0.2f);
    glm::vec3 m_S11LightColor = glm::vec3(1.0f);
    float m_S11LightIntensity = 1.5f;
    bool m_S11RainbowMode = false;

    // Scenario 12 UI variables
    std::string m_S12Status = "Ready";
    int m_S12RandomEntityCount = 0;

    // Scenario 13 UI variables
    std::string m_S13Status = "Ready";
    std::string m_S13NewAction = "PlayerJump";
    int m_S13NewKey = 32; // Space

    // Scenario 15 UI variables
    std::string m_S15Status = "Ready";
    int m_S15Level = 42;
    int m_S15XP = 12500;
    int m_S15EntityCount = 24;
    float m_S15EntitySize = 2.0f;

    // Scenario 16 UI variables
    std::string m_S16Status = "Stopped";
    bool m_S16IsServer = true;
    std::vector<std::string> m_S16Messages;
    float m_S16SendTimer = 0.0f;
    char m_S16MessageText[128] = "Hello from AxisEngine";
    char m_S16Host[64] = "127.0.0.1";
    int m_S16Port = 12345;

    // Scenario 17 UI variables
    std::shared_ptr<ISound> m_S17Audio2D = nullptr;
    std::shared_ptr<ISound> m_S17Audio3D = nullptr;
    float m_S17OrbitAngle = 0.0f;
    float m_S17Speed = 1.0f;
    float m_S17Volume2D = 0.35f;
    float m_S17Volume3D = 0.8f;
    float m_S17Pitch = 1.0f;
    float m_S17MinDistance = 2.0f;
    float m_S17MaxDistance = 50.0f;
    bool m_S17Play2D = false;
    bool m_S17Play3D = false;

    // Scenario 18 UI variables
    bool m_S18VideoPlaying = true;
    float m_S18Volume = 1.0f;

    // Scenario 19 UI variables
    int m_S19AnimIndex = 0;
    float m_S19Blend = 0.5f;
    float m_S19Speed = 1.0f;

    // Scenario 20 UI variables
    int m_S20ProbeResolution = 512;
    float m_S20Reflectivity = 0.65f;
    float m_S20FresnelBias = 0.04f;
    float m_S20FresnelPower = 5.0f;

    // Scenario 21 UI variables
    float m_S21GlassOpacity = 0.35f;
    float m_S21GlassRoughness = 0.12f;
    bool m_S21AnimateObjects = true;

    // Scenario 24/25 UI variables
    int m_S24LayerMask = 0x7;
    bool m_S25ReverseOrder = false;

    // Scenario 26 UI variables
    int m_S26InstanceCount = 5000;
    bool m_S26UniqueTint = false;

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

    // Benchmarking counters
    float m_FpsTime = 0.0f;
    int m_FpsCount = 0;
    float m_CurrentFps = 0.0f;

    // Scenario 5 navigation state
    entt::entity m_NavFollower = entt::null;
    std::vector<glm::vec3> m_NavWaypoints;
    int m_CurrentWaypointIndex = 0;

    // Cached entity handles (set in LoadScene*, used in OnUpdate to avoid per-frame O(n) scans)
    entt::entity m_S2DirLightEntity  = entt::null;
    entt::entity m_S2PointLightEntity = entt::null;
    entt::entity m_S2SpotLightEntity  = entt::null;
    entt::entity m_S11DirLightEntity  = entt::null;
};
