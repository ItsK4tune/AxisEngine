#pragma once

#include <core/logic/logger_types.h>
#include <core/type/audio_backend.h>
#include <core/type/graphics_backend.h>
#include <core/type/lighting_mode.h>
#include <core/type/physics_backend.h>
#include <core/type/physics_mode.h>
#include <core/type/render_path.h>
#include <core/type/spatial_culling_mode.h>
#include <core/type/tonemapping_mode.h>
#include <platform/type/window_mode.h>
#include <cstdint>
#include <string>
#include <utility>

#ifndef AXIS_HAS_IRRKLANG_BACKEND
#define AXIS_HAS_IRRKLANG_BACKEND 0
#endif
#ifndef AXIS_HAS_FMOD_BACKEND
#define AXIS_HAS_FMOD_BACKEND 0
#endif

struct WindowConfig
{
    int width = 800;
    int height = 600;
    WindowMode windowMode = WindowMode::Windowed;
    bool vsync = false;
    int monitorIndex = 0;
    int refreshRate = 0;
    int frameRateLimit = 0;
};

struct GraphicsConfig
{
    GraphicsBackend graphicsBackend = GraphicsBackend::OpenGL;
    int msaaSamples = 4;
    int antialiasing = 1;
    float maxAnisotropy = 16.0f;
    float renderScale = 1.0f;
    bool asyncResourceLoading = true;
    bool strictAssetLoading = false;
};

struct RenderConfig
{
    TonemappingMode tonemappingMode = TonemappingMode::ACES;
    bool hdrEnabled = false;
    bool bloomEnabled = false;
    float gamma = 2.2f;
    float exposure = 1.0f;
    float bloomIntensity = 1.0f;
    float bloomThreshold = 1.0f;
    float bloomRadius = 0.005f;
    float taaFeedback = 0.95f;
    float skyboxIntensity = 1.0f;
    float ambientIntensity = 1.0f;
    float uiReferenceWidth = 1920.0f;
    float uiReferenceHeight = 1080.0f;
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
};

struct ShadowConfig
{
    bool shadowsEnabled = true;
    int shadowMode = 1;
    int shadowMapResolution = 2048;
    float shadowProjectionSize = 100.0f;
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float shadowBias = 0.005f;
    int shadowSoftness = 1;
};

struct PhysicsConfig
{
    PhysicsBackend physicsBackend = PhysicsBackend::Bullet;
    PhysicsMode physicsMode = PhysicsMode::Balanced;
    float gravity[3] = {0.0f, -9.81f, 0.0f};
    int maxSubSteps = 10;
    float physicsTickRate = 60.0f;
    bool ccdEnabled = false;
    float ccdThreshold = 0.0f;
    int solverIterations = 10;
};

struct InputConfig
{
    float mouseSensitivityX = 0.1f;
    float mouseSensitivityY = 0.1f;
    bool mouseInvertX = false;
    bool mouseInvertY = false;
    bool rawMouseInput = true;
    float gamepadDeadZone = 0.15f;
};

struct AudioConfig
{
#if AXIS_HAS_IRRKLANG_BACKEND
    AudioBackend audioBackend = AudioBackend::IrrKlang;
#elif AXIS_HAS_FMOD_BACKEND
    AudioBackend audioBackend = AudioBackend::FMOD;
#else
    AudioBackend audioBackend = AudioBackend::Null;
#endif
    float masterVolume = 100.0f;
    // Playback endpoint preference. Backends that cannot switch endpoints must
    // report that capability instead of silently accepting this value.
    std::string audioDevice = "";
    bool captureEnabled = false;
    std::string captureDevice = "";
    float captureInputVolume = 1.0f;
    float captureNoiseGate = 0.02f;
    float captureGain = 4.0f;
    float captureAttackSeconds = 0.05f;
    float captureReleaseSeconds = 0.05f;
    float capturePeakDecaySeconds = 0.125f;
    float captureCalibrationSeconds = 1.0f;
    float capturePulseThreshold = 0.15f;
    float capturePulseCooldown = 0.08f;
    float capturePulseDuration = 0.6f;
};

struct CullingConfig
{
    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;
    bool stencilTestEnabled = true;
    bool frustumCullingEnabled = true;
    SpatialCullingMode spatialCullingMode = SpatialCullingMode::Auto;
    bool occlusionCullingEnabled = false;
    bool instanceBatchingEnabled = true;
    bool renderOrderEnabled = false;
    uint32_t filterLayerMask = 0xFFFFFFFF;
    float distanceCulling = 0.0f;
};

// Runtime performance policy. These values control optimizations that have a
// meaningful quality/latency trade-off; correctness-only optimizations remain
// implementation details and are intentionally not user-toggleable.
struct OptimizationConfig
{
    #if defined(NDEBUG)
    bool resourceHotReloadEnabled = false;
    #else
    bool resourceHotReloadEnabled = true;
    #endif
    bool resourceUploadBudgetEnabled = true;
    int maxModelUploadsPerFrame = 2;
    int maxTextureUploadsPerFrame = 4;
    bool discardCpuMeshDataAfterUpload = false;
    bool compressedTextureLoadingEnabled = true;

    bool streamingUpdateThrottlingEnabled = true;
    float streamingCheckIntervalSeconds = 1.0f;

    bool reflectionCaptureBudgetEnabled = true;
    int maxReflectionProbeFacesPerFrame = 2;
    int maxPlanarReflectionCapturesPerFrame = 1;

    bool shadowParallelBuildEnabled = true;
    int shadowParallelThreshold = 128;

    bool animationParallelEvaluationEnabled = true;
    int animationParallelThreshold = 64;

    bool navigationSpatialHashEnabled = true;
    float navigationAgentCellSize = 2.0f;
    bool navigationAsyncPathfindingEnabled = true;
    int navigationMaxPathRequestsPerFrame = 4;
    bool navMeshRebuildBudgetEnabled = true;
    int maxNavMeshRebuildsPerFrame = 1;
    bool navigationDirtyTilesEnabled = true;
    float navigationNavMeshTileSize = 8.0f;
    int navigationMaxDirtyTilesPerFrame = 4;

    bool networkBatchingEnabled = true;
    int networkMaxEventsPerUpdate = 256;
    float networkMaxEventProcessingMs = 2.0f;
    int networkMaxBytesPerUpdate = 1048576;
    bool networkReplicationEnabled = true;
    float networkReplicationRateHz = 20.0f;
    float networkInterestRadius = 0.0f;

    bool particleSpawnBudgetEnabled = true;
    int particleMaxSpawnPerFrame = 4096;
    bool particleBatchingEnabled = true;

    bool renderStateCacheEnabled = true;
    bool persistentMappedBuffersEnabled = true;
    bool tiledLightCullingEnabled = true;
    int tiledLightTileSize = 32;
    // Keep the picking attachment resident even when no built-in pass needs
    // it. Decals request it automatically; tools can request it per frame.
    bool gbufferEntityIdEnabled = false;
    bool physicsMeshShapeCacheEnabled = true;
    bool uiLayoutCacheEnabled = true;
    bool videoAsyncDecodeEnabled = true;
    int videoDecodeQueueSize = 3;
    float videoAVSyncThresholdSeconds = 0.20f;
    float videoLoadRetrySeconds = 1.0f;
};

struct DebugConfig
{
    bool physicsDebug = false;
    bool uiEnabled = true;
    bool gizmos = false;
    bool lightGizmos = false;
    bool entityNames = false;
    bool audioDebug = false;
    bool particleDebug = false;
    bool gridSnapEnabled = false;
    bool gridIndicatorEnabled = false;
    float gridSnapTranslation = 1.0f;
    float gridSnapRotation = 15.0f;
    float gridSnapScale = 0.25f;
};

struct AppConfig
{
    // Core
    std::string title = "Axis Engine";
    LogLevel logLevel = LogLevel::Debug;
    int numJobThreads = -1;
    float timeScale = 1.0f;
    std::string iconPath = "";
    bool headlessMode = false;
    bool loadDefaultAssets = true;
    std::string defaultAssetManifest = "asset://load.axs";

    // Sub-configs
    WindowConfig window;
    GraphicsConfig graphics;
    RenderConfig render;
    ShadowConfig shadow;
    PhysicsConfig physics;
    InputConfig input;
    AudioConfig audio;
    CullingConfig culling;
    OptimizationConfig optimization;
    DebugConfig debug;
    LightingMode lightingMode = LightingMode::RealTime;
};
