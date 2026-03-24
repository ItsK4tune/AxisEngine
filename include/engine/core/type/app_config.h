#pragma once

#include <cstdint>
#include <string>
#include <core/logic/logger_types.h>

#include <platform/type/window_mode.h>

#include <core/type/graphics_backend.h>
#include <core/type/physics_backend.h>
#include <core/type/audio_backend.h>
#include <core/type/tonemapping_mode.h>
#include <core/type/render_path.h>
#include <core/type/physics_mode.h>

struct AppConfig
{

    std::string title = "Axis Engine";
    LogLevel logLevel = LogLevel::Debug;
    int numJobThreads = -1;
    float timeScale = 1.0f;
    std::string iconPath = "";
    std::string audioDevice = "";


    int width = 800;
    int height = 600;
    WindowMode windowMode = WindowMode::Windowed;
    bool vsync = false;
    int monitorIndex = 0;
    int refreshRate = 0;
    int frameRateLimit = 0;


    GraphicsBackend graphicsBackend = GraphicsBackend::OpenGL;
    int msaaSamples = 4;
    int antialiasing = 1;
    float maxAnisotropy = 16.0f;
    float renderScale = 1.0f;
    bool asyncResourceLoading = true;
    

    RenderPath renderPath = RenderPath::Forward;
    TonemappingMode tonemappingMode = TonemappingMode::ACES;
    bool hdrEnabled = false;
    bool bloomEnabled = false;
    float gamma = 2.2f;
    float exposure = 1.0f;
    float bloomIntensity = 1.0f;
    float bloomThreshold = 1.0f;
    float bloomRadius = 0.005f;
    float skyboxIntensity = 1.0f;
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};


    bool shadowsEnabled = true;
    int shadowMode = 1;
    int shadowMapResolution = 2048;
    float shadowProjectionSize = 100.0f;
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float shadowBias = 0.005f;
    int shadowSoftness = 1;


    PhysicsBackend physicsBackend = PhysicsBackend::Bullet;
    PhysicsMode physicsMode = PhysicsMode::Balanced;
    float gravity[3] = {0.0f, -9.81f, 0.0f};
    int maxSubSteps = 10;
    float physicsTickRate = 60.0f;
    bool ccdEnabled = false;
    float ccdThreshold = 0.0f;
    int solverIterations = 10;


    float mouseSensitivityX = 0.1f;
    float mouseSensitivityY = 0.1f;
    bool mouseInvertX = false;
    bool mouseInvertY = false;


    AudioBackend audioBackend = AudioBackend::IrrKlang;
    float masterVolume = 10.0f;


    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    bool instanceBatchingEnabled = true;
    bool renderOrderEnabled = false;
    uint32_t filterLayerMask = 0xFFFFFFFF;
    float distanceCulling = 0.0f;
};