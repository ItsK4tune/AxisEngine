#include <app/config_loader.h>
#include <utils/logger.h>
#include <app/application.h>
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string Trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r\",{}");
    if (std::string::npos == first)
        return str;
    size_t last = str.find_last_not_of(" \t\n\r\",{}");
    return str.substr(first, (last - first + 1));
}

static std::string ExtractValue(const std::string &line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        return "";
    return Trim(line.substr(colonPos + 1));
}

static std::string ExtractKey(const std::string &line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        return "";
    return Trim(line.substr(0, colonPos));
}

AppConfig ConfigLoader::Load(const std::string &path)
{
    AppConfig config;
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOGGER_ERROR("ConfigLoader") << "Could not open config file: " << path << ". Using defaults.";
        return config;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string key = ExtractKey(line);
        std::string value = ExtractValue(line);

        if (key.empty())
            continue;

        if (key == "title")
            config.title = value;
        else if (key == "width")
            config.width = std::stoi(value);
        else if (key == "height")
            config.height = std::stoi(value);
        else if (key == "windowMode")
            config.windowMode = std::stoi(value);
        else if (key == "vsync")
            config.vsync = (value == "true");
        else if (key == "monitorIndex")
            config.monitorIndex = std::stoi(value);
        else if (key == "refreshRate")
            config.refreshRate = std::stoi(value);
        else if (key == "frameRateLimit")
            config.frameRateLimit = std::stoi(value);
        else if (key == "shadowMode")
            config.shadowMode = std::stoi(value);
        else if (key == "cullFaceEnabled")
            config.cullFaceEnabled = (value == "true");
        else if (key == "depthTestEnabled")
            config.depthTestEnabled = (value == "true");
        else if (key == "audioDevice")
            config.audioDevice = value;
        else if (key == "iconPath")
            config.iconPath = value;
        else if (key == "instanceBatchingEnabled")
            config.instanceBatchingEnabled = (value == "true");
        else if (key == "frustumCullingEnabled")
            config.frustumCullingEnabled = (value == "true");
        else if (key == "shadowProjectionSize")
            config.shadowProjectionSize = std::stof(value);
        else if (key == "shadowFrustumCullingEnabled")
            config.shadowFrustumCullingEnabled = (value == "true");
        else if (key == "shadowDistanceCulling")
            config.shadowDistanceCulling = std::stof(value);
        else if (key == "distanceCulling")
            config.distanceCulling = std::stof(value);
        else if (key == "physicsMode")
            config.physicsMode = std::stoi(value);
        else if (key == "width")
             config.width = std::stoi(value);
        else if (key == "antialiasing")
        {
            if (value == "FXAA") config.antialiasing = 1;
            else if (value == "TAA") config.antialiasing = 2;
            else config.antialiasing = 0;
        }
    }

    return config;
}

void ConfigLoader::LoadConfig(std::stringstream &ss, Application *app)
{
    std::string subCmd;
    ss >> subCmd;
    if (subCmd == "SHADOWS")
    {
        int mode = 1;
        ss >> mode;
        if (app)
        {
            app->GetRenderSystem().SetShadowMode(mode);
        }
    }
    else if (subCmd == "SHADOW_SIZE")
    {
        float size = 20.0f;
        ss >> size;
        if (app)
        {
            app->GetRenderSystem().SetShadowProjectionSize(size);
        }
    }
    else if (subCmd == "INSTANCING")
    {
        int enable = 0;
        ss >> enable;
        if (app)
        {
            app->GetRenderSystem().SetInstanceBatching(enable != 0);
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
            if (modeStr == "FRONT")
                mode = GL_FRONT;
            else if (modeStr == "FRONT_AND_BACK")
                mode = GL_FRONT_AND_BACK;
            else if (modeStr != "BACK")
                LOGGER_WARN("ConfigLoader") << "Invalid CULL_FACE mode: " << modeStr << ". Supported: BACK, FRONT, FRONT_AND_BACK.";

            if (app)
                app->GetRenderSystem().SetFaceCulling(true, mode);
        }
        else
        {
            if (app)
                app->GetRenderSystem().SetFaceCulling(false);
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
            if (funcStr == "NEVER")
                func = GL_NEVER;
            else if (funcStr == "LESS")
                func = GL_LESS;
            else if (funcStr == "EQUAL")
                func = GL_EQUAL;
            else if (funcStr == "LEQUAL")
                func = GL_LEQUAL;
            else if (funcStr == "GREATER")
                func = GL_GREATER;
            else if (funcStr == "NOTEQUAL")
                func = GL_NOTEQUAL;
            else if (funcStr == "GEQUAL")
                func = GL_GEQUAL;
            else if (funcStr == "ALWAYS")
                func = GL_ALWAYS;
            else 
                LOGGER_WARN("ConfigLoader") << "Invalid DEPTH_TEST func: " << funcStr << ". Supported: NEVER, LESS, EQUAL, LEQUAL, GREATER, NOTEQUAL, GEQUAL, ALWAYS.";

            if (app)
                app->GetRenderSystem().SetDepthTest(true, func);
        }
        else
        {
            if (app)
                app->GetRenderSystem().SetDepthTest(false);
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
                if (modeStr == "FULLSCREEN")
                    mode = WindowMode::FULLSCREEN;
                else if (modeStr == "BORDERLESS")
                    mode = WindowMode::BORDERLESS;
                else if (modeStr == "WINDOWED")
                    mode = WindowMode::WINDOWED;
                else
                    LOGGER_WARN("ConfigLoader") << "Invalid WINDOW mode: " << modeStr << ". Supported: WINDOWED, FULLSCREEN, BORDERLESS.";
            }

            if (!ss.eof())
                ss >> monitorIdx;

            int refreshRate = 0;
            if (!ss.eof())
                ss >> refreshRate;

            if (app)
                app->GetMonitorManager().SetWindowConfiguration(w, h, mode, monitorIdx, refreshRate);
        }
    }
    else if (subCmd == "VSYNC")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (app)
                app->GetMonitorManager().SetVsync(enable != 0);
        }
    }
    else if (subCmd == "FPS")
    {
        int fps = 0;
        if (ss >> fps)
        {
            if (app)
                app->GetMonitorManager().SetFrameRateLimit(fps);
        }
    }
    else if (subCmd == "FRUSTUM")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (app)
                app->GetRenderSystem().SetFrustumCulling(enable != 0);
        }
    }
    else if (subCmd == "SHADOW_FRUSTUM")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (app)
                app->GetRenderSystem().SetShadowFrustumCulling(enable != 0);
        }
    }
    else if (subCmd == "SHADOW_DISTANCE")
    {
        float dist = 0.0f;
        if (ss >> dist)
        {
            if (app)
                app->GetRenderSystem().SetShadowDistanceCulling(dist);
        }
    }
    else if (subCmd == "DISTANCE")
    {
        float dist = 0.0f;
        if (ss >> dist)
        {
            if (app)
                app->GetRenderSystem().SetDistanceCulling(dist);
        }
    }
    else if (subCmd == "PHYSICS_MODE")
    {
        std::string modeStr;
        int mode = 1;
        if (ss >> modeStr)
        {
            if (modeStr == "FAST") mode = 0;
            else if (modeStr == "BALANCED") mode = 1;
            else if (modeStr == "ACCURATE") mode = 2;
            else 
            {
                try { mode = std::stoi(modeStr); } 
                catch(...) { 
                    LOGGER_WARN("ConfigLoader") << "Invalid PHYSICS_MODE: " << modeStr << ". Supported: FAST, BALANCED, ACCURATE (or 0, 1, 2).";
                }
            }
            
            if (app)
            {
                app->GetPhysicsWorld().SetMode(mode);
            }
        }
    }
    else if (subCmd == "PHYSICS_ASYNC")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            bool async = (valStr == "TRUE" || valStr == "true" || valStr == "1");
            if (app) 
            {
                app->GetPhysicsSystem().SetAsyncPhysics(async);
            }
        }
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            AntiAliasingMode mode = AntiAliasingMode::NONE;
            if (valStr == "FXAA") mode = AntiAliasingMode::FXAA;
            else if (valStr == "TAA") mode = AntiAliasingMode::TAA;
            else if (valStr == "NONE") mode = AntiAliasingMode::NONE;
            else
                LOGGER_WARN("ConfigLoader") << "Invalid ANTIALIASING mode: " << valStr << ". Supported: NONE, FXAA, TAA.";
            
            if (app)
            {
                app->GetRenderSystem().SetAntiAliasingMode(mode);
            }
        }
    }
}
