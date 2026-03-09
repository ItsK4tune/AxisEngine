#pragma once

#include <cstdint>
#include <string>

struct AppConfig
{
    std::string title = "Axis Engine";
    int width = 800;
    int height = 600;
    int windowMode = 0;
    bool vsync = false;
    int monitorIndex = 0;
    int refreshRate = 0;
    int frameRateLimit = 0;

    // Rendering & Post-Processing
    int shadowMode = 1;
    bool shadowsEnabled = true;
    bool bloomEnabled = false;
    bool hdrEnabled = false;
    bool taaEnabled = false;
    float gamma = 2.2f;
    float exposure = 1.0f;
    float bloomIntensity = 1.0f;
    float taaBlendFactor = 0.9f;
    float ambientIntensity = 0.1f;
    int tonemappingMode = 1; // 0: None, 1: ACES, 2: Reinhard
    int renderPath = 0;      // 0: Forward, 1: Deferred
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;

    bool renderOrderEnabled = false;
    uint32_t filterLayerMask = 0xFFFFFFFF;

    // Audio
    std::string audioDevice = "default";
    float masterVolume = 1.0f;

    std::string iconPath = "includes/engine/asset/project/icon.png";

    // Optimization
    bool instanceBatchingEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    float shadowProjectionSize = 100.0f;
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float distanceCulling = 0.0f;
    int antialiasing = 1;

    // Core
    int numJobThreads = -1; // -1 for hardware_concurrency
    int logLevel = 1;       // 0: Debug, 1: Info, 2: Warn, 3: Error
    float timeScale = 1.0f;

    // Physics
    int physicsMode = 1;
    float gravity[3] = {0.0f, -9.81f, 0.0f};
    int maxSubSteps = 10;
    float physicsTickRate = 60.0f;
    bool ccdEnabled = false;
    int solverIterations = 10;

    // Navigation
    float agentRadius = 0.5f;
    float agentHeight = 2.0f;
    std::string walkableTag = "Walkable";

    std::string graphicsBackend = "OPENGL";
    std::string physicsBackend = "BULLET";
    std::string audioBackend = "IRRKLANG";
};