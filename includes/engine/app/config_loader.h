#pragma once

#include <string>
#include <iostream>

#include <sstream>

class Application;

struct AppConfig
{
    std::string title = "Game Engine";
    int width = 800;
    int height = 600;
    int windowMode = 0;
    bool vsync = false;
    int monitorIndex = 0;
    int refreshRate = 0;
    int frameRateLimit = 0;
    
    bool shadowsEnabled = true;
    bool cullFaceEnabled = true;
    bool depthTestEnabled = true;
    
    std::string audioDevice = "default";

    std::string iconPath = "";
};

class ConfigLoader
{
public:
    static AppConfig Load(const std::string& path);
    static void LoadConfig(std::stringstream& ss, Application* app);
};
