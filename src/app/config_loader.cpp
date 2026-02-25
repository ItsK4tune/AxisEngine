#include <app/config_loader.h>
#include <utils/logger.h>
#include <interface/graphic/graphics_types.h>
#include <interface/window/i_window.h>
#include <app/application.h>
#include <app/monitor_manager.h>
#include <ecs/systems/render_system.h>
#include <interface/physics/i_physics_world.h>
#include <graphic/core/post_process_pipeline.h>
#include <algorithm>

void ConfigLoader::LoadConfig(std::stringstream &ss, AppConfig &config)
{
    std::string subCmd;
    ss >> subCmd;

    if (subCmd == "GRAPHICS_API")
    {
        std::string backend;
        if (ss >> backend)
        {
            if (backend == "OPENGL")
            {
                config.graphicsBackend = "OPENGL";
            }
            else if (backend == "VULKAN" || backend == "DIRECTX")
            {
                LOGGER_WARN("ConfigLoader") << "Graphics Backend '" << backend << "' is not yet implemented. Switching back to OPENGL.";
                config.graphicsBackend = "OPENGL";
            }
            else
            {
                LOGGER_WARN("ConfigLoader") << "Unknown Graphics Backend '" << backend << "'. Defaulting to OPENGL.";
                config.graphicsBackend = "OPENGL";
            }
        }
    }
    else if (subCmd == "PHYSICS_ENGINE")
    {
        std::string backend;
        if (ss >> backend)
        {
            if (backend == "BULLET")
            {
                config.physicsBackend = "BULLET";
            }
            else if (backend == "PHYSX")
            {
                LOGGER_WARN("ConfigLoader") << "Physics Engine '" << backend << "' is not yet implemented. Switching back to BULLET.";
                config.physicsBackend = "BULLET";
            }
            else
            {
                LOGGER_WARN("ConfigLoader") << "Unknown Physics Engine '" << backend << "'. Defaulting to BULLET.";
                config.physicsBackend = "BULLET";
            }
        }
    }
    else if (subCmd == "AUDIO_ENGINE")
    {
        std::string backend;
        if (ss >> backend)
        {
            if (backend == "IRRKLANG")
            {
                config.audioBackend = "IRRKLANG";
            }
            else
            {
                LOGGER_WARN("ConfigLoader") << "Audio Engine '" << backend << "' is not yet implemented (or unknown). Switching back to IRRKLANG.";
                config.audioBackend = "IRRKLANG";
            }
        }
    }

    else if (subCmd == "SHADOWS")
    {
        int mode = 1;
        ss >> mode;
        config.shadowMode = mode;
    }
    else if (subCmd == "SHADOW_SIZE")
    {
        float size = 20.0f;
        ss >> size;
        config.shadowProjectionSize = size;
    }
    else if (subCmd == "INSTANCING")
    {
        int enable = 0;
        ss >> enable;
        config.instanceBatchingEnabled = (enable != 0);
    }
     else if (subCmd == "CULL_FACE")
    {
        int enable = 0;
        ss >> enable;
        config.cullFaceEnabled = (enable != 0);

    }
    else if (subCmd == "OCCLUSION_CULLING")
    {
        int enable = 0;
        ss >> enable;
        config.occlusionCullingEnabled = (enable != 0);
    }
    else if (subCmd == "DEPTH_TEST")
    {
        int enable = 0;
        ss >> enable;
        config.depthTestEnabled = (enable != 0);
    }
    else if (subCmd == "WINDOW_WIDTH") {
        int w = 800; ss >> w; config.width = w;
    }
    else if (subCmd == "WINDOW_HEIGHT") {
        int h = 600; ss >> h; config.height = h;
    }
    else if (subCmd == "WINDOW_MODE") {
        std::string modeStr;
        if (ss >> modeStr) {
            if (modeStr == "FULLSCREEN") config.windowMode = 1;
            else if (modeStr == "BORDERLESS") config.windowMode = 2;
            else if (modeStr == "BORDERLESS_FULLSCREEN" || modeStr == "STRETCH_FULLSCREEN") config.windowMode = 3;
            else if (modeStr == "WINDOWED") config.windowMode = 0;
            else config.windowMode = 0;
        }
    }
    else if (subCmd == "WINDOW_MONITOR") {
        int monitorIdx = 0; ss >> monitorIdx; config.monitorIndex = monitorIdx;
    }
    else if (subCmd == "WINDOW_REFRESH_RATE") {
        int rate = 0; ss >> rate; config.refreshRate = rate;
    }
    else if (subCmd == "VSYNC")
    {
        int enable = 0;
        ss >> enable;
        config.vsync = (enable != 0);
    }
    else if (subCmd == "FPS")
    {
        int fps = 0;
        ss >> fps;
        config.frameRateLimit = fps;
    }
    else if (subCmd == "FRUSTUM")
    {
        int enable = 0;
        ss >> enable;
        config.frustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "RENDER_ORDER")
    {
        int enable = 0;
        ss >> enable;
        config.renderOrderEnabled = (enable != 0);
    }
    else if (subCmd == "FILTER_LAYER")
    {
        uint32_t mask = 0xFFFFFFFF;
        ss >> mask;
        config.filterLayerMask = mask;
    }
    else if (subCmd == "SHADOW_FRUSTUM")
    {
        int enable = 0;
        ss >> enable;
        config.shadowFrustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "SHADOW_DISTANCE")
    {
        float dist = 0.0f;
        ss >> dist;
        config.shadowDistanceCulling = dist;
    }
    else if (subCmd == "DISTANCE")
    {
        float dist = 0.0f;
        ss >> dist;
        config.distanceCulling = dist;
    }
    else if (subCmd == "PHYSICS_MODE")
    {
        std::string modeStr;
        if (ss >> modeStr)
        {
            if (modeStr == "FAST") config.physicsMode = 0;
            else if (modeStr == "BALANCED") config.physicsMode = 1;
            else if (modeStr == "ACCURATE") config.physicsMode = 2;
            else
            {
                try { config.physicsMode = std::stoi(modeStr); }
                catch(...) { config.physicsMode = 1; }
            }
        }
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            if (valStr == "FXAA") config.antialiasing = 1;
            else if (valStr == "TAA") config.antialiasing = 2;
            else config.antialiasing = 0;
        }
    }
}

void ConfigLoader::LoadConfig(std::stringstream &ss, Application* app)
{

    std::string subCmd;

    ss >> subCmd;
    if (subCmd == "GRAPHICS_API")
    {
        std::string backend;
        if (ss >> backend && app) {
            AppConfig& cfg = const_cast<AppConfig&>(app->GetConfig());
            cfg.graphicsBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Graphics Backend: " << backend;
        }
    }
    else if (subCmd == "PHYSICS_ENGINE")
    {
        std::string backend;
        if (ss >> backend && app) {
            AppConfig& cfg = const_cast<AppConfig&>(app->GetConfig());
            cfg.physicsBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Physics Backend: " << backend;
        }
    }
    else if (subCmd == "AUDIO_ENGINE")
    {
        std::string backend;
        if (ss >> backend && app) {
            AppConfig& cfg = const_cast<AppConfig&>(app->GetConfig());
            cfg.audioBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Audio Backend: " << backend;
        }
    }
    else if (subCmd == "SHADOWS")
    {
        int mode = 1;
        ss >> mode;
        if (app) app->GetRenderSystem().SetShadowMode(mode);
    }
    else if (subCmd == "SHADOW_SIZE")
    {
        float size = 20.0f;
        ss >> size;
        if (app) app->GetRenderSystem().SetShadowProjectionSize(size);
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
    else if (subCmd.find("WINDOW") != std::string::npos)
    {
        // Window configurations are applied in batch by the SceneSerializer
        // to avoid recreating the window multiple times during parsing.
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
    else if (subCmd == "RENDER_ORDER")
    {
        int enable = 0;
        if (ss >> enable && app)
        {
            bool renderOrder = (enable != 0);
            app->GetRenderSystem().SetRenderOrderEnabled(renderOrder);

            if (renderOrder)
            {
                // Disable depth test when explicit render order is active
                app->GetRenderSystem().SetDepthTest(false);
            }
            else
            {
                // Restore depth test based on configuration when render order is disabled
                app->GetRenderSystem().SetDepthTest(app->GetConfig().depthTestEnabled);
            }
            
            // Sync config state
            AppConfig& cfg = const_cast<AppConfig&>(app->GetConfig());
            cfg.renderOrderEnabled = renderOrder;
        }
    }
    else if (subCmd == "FILTER_LAYER")
    {
        uint32_t mask = 0xFFFFFFFF;
        if (ss >> mask)
        {
            if (app)
                app->GetRenderSystem().SetFilterLayerMask(mask);
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
