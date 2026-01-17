#include <app/config_loader.h>
#include <fstream>
#include <sstream>
#include <algorithm>

// Minimal JSON Parser helper
// Handles simple "key": value or "key": "value" lines.
// Does NOT support nested objects or arrays or multi-line values robustly.

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\",{}");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r\",{}");
    return str.substr(first, (last - first + 1));
}

static std::string ExtractValue(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return "";
    return Trim(line.substr(colonPos + 1));
}

static std::string ExtractKey(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return "";
    return Trim(line.substr(0, colonPos));
}

AppConfig ConfigLoader::Load(const std::string& path)
{
    AppConfig config;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ConfigLoader] Could not open config file: " << path << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string key = ExtractKey(line);
        std::string value = ExtractValue(line);
        
        if (key.empty()) continue;

        if (key == "title") config.title = value;
        else if (key == "width") config.width = std::stoi(value);
        else if (key == "height") config.height = std::stoi(value);
        else if (key == "windowMode") config.windowMode = std::stoi(value);
        else if (key == "vsync") config.vsync = (value == "true");
        else if (key == "monitorIndex") config.monitorIndex = std::stoi(value);
        else if (key == "refreshRate") config.refreshRate = std::stoi(value);
        else if (key == "frameRateLimit") config.frameRateLimit = std::stoi(value);
        else if (key == "shadowsEnabled") config.shadowsEnabled = (value == "true");
        else if (key == "cullFaceEnabled") config.cullFaceEnabled = (value == "true");
        else if (key == "depthTestEnabled") config.depthTestEnabled = (value == "true");
        else if (key == "audioDevice") config.audioDevice = value;
        else if (key == "iconPath") config.iconPath = value;
    }
    
    return config;
}

#include <app/application.h>

void ConfigLoader::LoadConfig(std::stringstream& ss, Application* app)
{
    std::string subCmd;
    ss >> subCmd;
    if (subCmd == "SHADOWS")
    {
        int enable = 0;
        ss >> enable;
        if (app)
        {
            app->GetRenderSystem().SetEnableShadows(enable != 0);
        }
    }
    else if (subCmd == "CULL_FACE")
    {
        int enable = 0;
        std::string modeStr;
        ss >> enable;
        if (enable)
        {
            ss >> modeStr;
            int mode = GL_BACK;
            if (modeStr == "FRONT") mode = GL_FRONT;
            else if (modeStr == "FRONT_AND_BACK") mode = GL_FRONT_AND_BACK;

            if (app) app->GetRenderSystem().SetFaceCulling(true, mode);
        }
        else
        {
            if (app) app->GetRenderSystem().SetFaceCulling(false);
        }
    }
    else if (subCmd == "DEPTH_TEST")
    {
        int enable = 0;
        std::string funcStr;
        ss >> enable;
        if (enable)
        {
            ss >> funcStr;
            int func = GL_LESS;
            if (funcStr == "NEVER") func = GL_NEVER;
            else if (funcStr == "LESS") func = GL_LESS;
            else if (funcStr == "EQUAL") func = GL_EQUAL;
            else if (funcStr == "LEQUAL") func = GL_LEQUAL;
            else if (funcStr == "GREATER") func = GL_GREATER;
            else if (funcStr == "NOTEQUAL") func = GL_NOTEQUAL;
            else if (funcStr == "GEQUAL") func = GL_GEQUAL;
            else if (funcStr == "ALWAYS") func = GL_ALWAYS;

            if (app) app->GetRenderSystem().SetDepthTest(true, func);
        }
        else
        {
            if (app) app->GetRenderSystem().SetDepthTest(false);
        }
    }
    else if (subCmd == "WINDOW")
    {
        int w, h;
        if (ss >> w >> h)
        {
            std::string modeStr;
            int monitorIdx = 0;
            WindowMode mode = WindowMode::WINDOWED;

            if (ss >> modeStr)
            {
                if (modeStr == "FULLSCREEN") mode = WindowMode::FULLSCREEN;
                else if (modeStr == "BORDERLESS") mode = WindowMode::BORDERLESS;
                else if (modeStr == "WINDOWED") mode = WindowMode::WINDOWED;
            }

            if (!ss.eof())
                ss >> monitorIdx;

            int refreshRate = 0;
            if (!ss.eof())
                ss >> refreshRate;

            if (app) app->GetMonitorManager().SetWindowConfiguration(w, h, mode, monitorIdx, refreshRate);
        }
    }
    else if (subCmd == "VSYNC")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (app) app->GetMonitorManager().SetVsync(enable != 0);
        }
    }
    else if (subCmd == "FPS")
    {
        int fps = 0;
        if (ss >> fps)
        {
            if (app) app->GetMonitorManager().SetFrameRateLimit(fps);
        }
    }
}
