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

#include "scriptable/all_scriptables.h"

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

#ifdef AXIS_TESTING
public:
#else
private:
#endif
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
    AudioSourceComponent* FindScenario31AudioSource(const char* name);
    void ApplyScenario31AudioSettings();

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
    bool m_S1RandomizePositions = false;

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

    // Scenario 31 UI variables
    static constexpr float kScenario31MaxVolume = 100.0f;
    std::shared_ptr<ISound> m_S31Audio2D = nullptr;
    std::shared_ptr<ISound> m_S31Audio3D = nullptr;
    float m_S31OrbitAngle = 0.0f;
    float m_S31Speed = 1.0f;
    float m_S31Volume2D = 35.0f;
    float m_S31Volume3D = 80.0f;
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
    bool m_S16FlipTextureY = true;
    int m_S16LayoutMode = 0;
    float m_S16PanelAlpha = 0.92f;
    bool m_S16WordWrap = true;
    float m_S16ShowcaseX = 630.0f;
    float m_S16ShowcaseW = 620.0f;
    float m_S16ShowcaseH = 220.0f;
    float m_S16ShowcaseRot = 0.0f;
    float m_S16ShowcaseScale = 1.0f;
    bool m_S16ShowcaseAnim = false;

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
    entt::entity m_S4ForwardSphereEntity = entt::null;
    entt::entity m_S4LightEntity = entt::null;
    entt::entity m_S16CardEntity = entt::null;
    entt::entity m_S16TextureEntity = entt::null;
    entt::entity m_S16RootPanel = entt::null;
};
