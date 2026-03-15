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
    // General
    std::string title = "Axis Engine";
    LogLevel logLevel = LogLevel::Debug;
    int numJobThreads = -1; // -1 for hardware_concurrency
    float timeScale = 1.0f;
    std::string iconPath = "";
    std::string audioDevice = "";

    // Window / Display
    int width = 800;
    int height = 600;
    WindowMode windowMode = WindowMode::Windowed;
    bool vsync = false;
    int monitorIndex = 0;
    int refreshRate = 0;
    int frameRateLimit = 0;

    // Graphics (Global)
    GraphicsBackend graphicsBackend = GraphicsBackend::OpenGL;
    int msaaSamples = 4;
    int antialiasing = 1; // 0: None, 1: FXAA, 2: TAA
    float maxAnisotropy = 16.0f;
    float renderScale = 1.0f;
    bool asyncResourceLoading = true;
    
    // Rendering & Effects
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
    bool fogEnabled = false;
    float fogColor[3] = {0.5f, 0.5f, 0.5f};
    float fogDensity = 0.01f;
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

    // Shadows
    bool shadowsEnabled = true;
    int shadowMode = 1;
    int shadowMapResolution = 2048;
    float shadowProjectionSize = 100.0f;
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float shadowBias = 0.005f;
    int shadowSoftness = 1; // 0: Hard, 1: 3x3 PCF, 2: 5x5 PCF

    // Physics
    PhysicsBackend physicsBackend = PhysicsBackend::Bullet;
    PhysicsMode physicsMode = PhysicsMode::Balanced;
    float gravity[3] = {0.0f, -9.81f, 0.0f};
    int maxSubSteps = 10;
    float physicsTickRate = 60.0f;
    bool ccdEnabled = false;
    float ccdThreshold = 0.0f;
    int solverIterations = 10;

    // Input
    float mouseSensitivityX = 0.1f;
    float mouseSensitivityY = 0.1f;
    bool mouseInvertX = false;
    bool mouseInvertY = false;

    // Audio
    AudioBackend audioBackend = AudioBackend::IrrKlang;
    float masterVolume = 1.0f;

    // Low-level Culling & State
    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    bool instanceBatchingEnabled = true;
    bool renderOrderEnabled = false;
    uint32_t filterLayerMask = 0xFFFFFFFF;
    float distanceCulling = 0.0f;
};