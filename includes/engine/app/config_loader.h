#pragma once

#include <string>
#include <iostream>

#include <sstream>

class Application;

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

    int shadowMode = 1;
    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;

    std::string audioDevice = "default";

    std::string iconPath = "includes/engine/asset/project/icon.png";

    bool instanceBatchingEnabled = true;
    bool frustumCullingEnabled = true;
    bool occlusionCullingEnabled = false;
    float shadowProjectionSize = 100.0f;
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float distanceCulling = 0.0f;
    int antialiasing = 1;

    int physicsMode = 1;

    std::string graphicsBackend = "OPENGL";
    std::string physicsBackend = "BULLET";
    std::string audioBackend = "IRRKLANG";
};

class ConfigLoader
{
public:
    static void LoadConfig(std::stringstream &ss, std::shared_ptr<Application> app);
    static void LoadConfig(std::stringstream &ss, AppConfig &config);
};
