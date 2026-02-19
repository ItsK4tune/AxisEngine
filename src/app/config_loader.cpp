#include <app/config_loader.h>
#include <utils/logger.h>
#include <interface/graphic/graphics_types.h>
#include <interface/window/i_window.h>
#include <app/application.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>

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
            Graphics::CullMode mode = Graphics::CullMode::Back; 
            if (modeStr == "FRONT")
                mode = Graphics::CullMode::Front;
            else if (modeStr == "FRONT_AND_BACK")
                mode = Graphics::CullMode::FrontAndBack;
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
            Graphics::CompareFunc func = Graphics::CompareFunc::Less; 
            if (funcStr == "NEVER")
                func = Graphics::CompareFunc::Never;
            else if (funcStr == "LESS")
                func = Graphics::CompareFunc::Less;
            else if (funcStr == "EQUAL")
                func = Graphics::CompareFunc::Equal;
            else if (funcStr == "LEQUAL")
                func = Graphics::CompareFunc::Lequal;
            else if (funcStr == "GREATER")
                func = Graphics::CompareFunc::Greater;
            else if (funcStr == "NOTEQUAL")
                func = Graphics::CompareFunc::NotEqual;
            else if (funcStr == "GEQUAL")
                func = Graphics::CompareFunc::Gequal;
            else if (funcStr == "ALWAYS")
                func = Graphics::CompareFunc::Always;
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
            WindowMode mode = WindowMode::Windowed;

            if (ss >> modeStr)
            {
                if (modeStr == "FULLSCREEN")
                    mode = WindowMode::Fullscreen;
                else if (modeStr == "BORDERLESS")
                    mode = WindowMode::Borderless;
                else if (modeStr == "WINDOWED")
                    mode = WindowMode::Windowed;
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
