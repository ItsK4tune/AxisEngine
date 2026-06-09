#pragma once

#include <core/logic/logger_types.h>
#include <core/type/audio_backend.h>
#include <core/type/graphics_backend.h>
#include <core/type/lighting_mode.h>
#include <core/type/physics_backend.h>
#include <core/type/physics_mode.h>
#include <core/type/render_path.h>
#include <core/type/tonemapping_mode.h>
#include <platform/type/window_mode.h>
#include <cstdint>
#include <string>
#include <utility>

#ifndef AXIS_HAS_IRRKLANG_BACKEND
#define AXIS_HAS_IRRKLANG_BACKEND 1
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
};

struct AudioConfig
{
#if AXIS_HAS_IRRKLANG_BACKEND
    AudioBackend audioBackend = AudioBackend::IrrKlang;
#else
    AudioBackend audioBackend = AudioBackend::Null;
#endif
    float masterVolume = 100.0f;
    std::string audioDevice = "";
};

struct CullingConfig
{
    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    bool instanceBatchingEnabled = true;
    bool renderOrderEnabled = false;
    uint32_t filterLayerMask = 0xFFFFFFFF;
    float distanceCulling = 0.0f;
};

struct DebugConfig
{
    bool wireframeMode = false;
    bool noTexture = false;
    bool physicsDebug = false;
#ifdef ENABLE_EDITOR
    bool uiEnabled = true;
#else
    bool uiEnabled = true;
#endif
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

    // Sub-configs
    WindowConfig window;
    GraphicsConfig graphics;
    RenderConfig render;
    ShadowConfig shadow;
    PhysicsConfig physics;
    InputConfig input;
    AudioConfig audio;
    CullingConfig culling;
    DebugConfig debug;
    LightingMode lightingMode = LightingMode::RealTime;

    // Legacy accessors keep existing code compiling during config migration.
    int& width = window.width;
    int& height = window.height;
    WindowMode& windowMode = window.windowMode;
    bool& vsync = window.vsync;
    int& monitorIndex = window.monitorIndex;
    int& refreshRate = window.refreshRate;
    int& frameRateLimit = window.frameRateLimit;

    GraphicsBackend& graphicsBackend = graphics.graphicsBackend;
    int& msaaSamples = graphics.msaaSamples;
    int& antialiasing = graphics.antialiasing;
    float& maxAnisotropy = graphics.maxAnisotropy;
    float& renderScale = graphics.renderScale;
    bool& asyncResourceLoading = graphics.asyncResourceLoading;
    bool& strictAssetLoading = graphics.strictAssetLoading;

    TonemappingMode& tonemappingMode = render.tonemappingMode;
    bool& hdrEnabled = render.hdrEnabled;
    bool& bloomEnabled = render.bloomEnabled;
    float& gamma = render.gamma;
    float& exposure = render.exposure;
    float& bloomIntensity = render.bloomIntensity;
    float& bloomThreshold = render.bloomThreshold;
    float& bloomRadius = render.bloomRadius;
    float& skyboxIntensity = render.skyboxIntensity;
    float& ambientIntensity = render.ambientIntensity;
    float& uiReferenceWidth = render.uiReferenceWidth;
    float& uiReferenceHeight = render.uiReferenceHeight;
    float (&clearColor)[4] = render.clearColor;

    bool& shadowsEnabled = shadow.shadowsEnabled;
    int& shadowMode = shadow.shadowMode;
    int& shadowMapResolution = shadow.shadowMapResolution;
    float& shadowProjectionSize = shadow.shadowProjectionSize;
    bool& shadowFrustumCullingEnabled = shadow.shadowFrustumCullingEnabled;
    float& shadowDistanceCulling = shadow.shadowDistanceCulling;
    float& shadowBias = shadow.shadowBias;
    int& shadowSoftness = shadow.shadowSoftness;

    PhysicsBackend& physicsBackend = physics.physicsBackend;
    PhysicsMode& physicsMode = physics.physicsMode;
    float (&gravity)[3] = physics.gravity;
    int& maxSubSteps = physics.maxSubSteps;
    float& physicsTickRate = physics.physicsTickRate;
    bool& ccdEnabled = physics.ccdEnabled;
    float& ccdThreshold = physics.ccdThreshold;
    int& solverIterations = physics.solverIterations;

    float& mouseSensitivityX = input.mouseSensitivityX;
    float& mouseSensitivityY = input.mouseSensitivityY;
    bool& mouseInvertX = input.mouseInvertX;
    bool& mouseInvertY = input.mouseInvertY;
    bool& rawMouseInput = input.rawMouseInput;

    AudioBackend& audioBackend = audio.audioBackend;
    float& masterVolume = audio.masterVolume;
    std::string& audioDevice = audio.audioDevice;

    bool& cullFaceEnabled = culling.cullFaceEnabled;
    bool& depthTestEnabled = culling.depthTestEnabled;
    bool& frustumCullingEnabled = culling.frustumCullingEnabled;
    bool& occlusionCullingEnabled = culling.occlusionCullingEnabled;
    bool& instanceBatchingEnabled = culling.instanceBatchingEnabled;
    bool& renderOrderEnabled = culling.renderOrderEnabled;
    uint32_t& filterLayerMask = culling.filterLayerMask;
    float& distanceCulling = culling.distanceCulling;

    // Copy/move support (references need special handling)
    AppConfig() = default;
    AppConfig(const AppConfig& o)
        : title(o.title),
          logLevel(o.logLevel),
          numJobThreads(o.numJobThreads),
          timeScale(o.timeScale),
          iconPath(o.iconPath),
          headlessMode(o.headlessMode),
          window(o.window),
          graphics(o.graphics),
          render(o.render),
          shadow(o.shadow),
          physics(o.physics),
          input(o.input),
          audio(o.audio),
          culling(o.culling),
          debug(o.debug),
          lightingMode(o.lightingMode)
    {
    }

    AppConfig& operator=(const AppConfig& o)
    {
        if (this == &o)
            return *this;
        title = o.title;
        logLevel = o.logLevel;
        numJobThreads = o.numJobThreads;
        timeScale = o.timeScale;
        iconPath = o.iconPath;
        headlessMode = o.headlessMode;
        window = o.window;
        graphics = o.graphics;
        render = o.render;
        shadow = o.shadow;
        physics = o.physics;
        input = o.input;
        audio = o.audio;
        culling = o.culling;
        debug = o.debug;
        lightingMode = o.lightingMode;
        return *this;
    }

    AppConfig(AppConfig&& o) noexcept
        : AppConfig()
    {
        *this = std::move(o);
    }

    AppConfig& operator=(AppConfig&& o) noexcept
    {
        if (this == &o)
            return *this;
        title = std::move(o.title);
        logLevel = o.logLevel;
        numJobThreads = o.numJobThreads;
        timeScale = o.timeScale;
        iconPath = std::move(o.iconPath);
        headlessMode = o.headlessMode;
        window = std::move(o.window);
        graphics = std::move(o.graphics);
        render = std::move(o.render);
        shadow = std::move(o.shadow);
        physics = std::move(o.physics);
        input = std::move(o.input);
        audio = std::move(o.audio);
        culling = std::move(o.culling);
        debug = std::move(o.debug);
        lightingMode = o.lightingMode;
        return *this;
    }
};
