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

    std::string iconPath = "";
    
    bool instanceBatchingEnabled = true;
    bool frustumCullingEnabled = true;
    float shadowProjectionSize = 100.0f; 
    bool shadowFrustumCullingEnabled = true;
    float shadowDistanceCulling = 100.0f;
    float distanceCulling = 0.0f; // 0 = disabled
};

class ConfigLoader
{
public:
    static AppConfig Load(const std::string& path);
    static void LoadConfig(std::stringstream& ss, Application* app);
};
