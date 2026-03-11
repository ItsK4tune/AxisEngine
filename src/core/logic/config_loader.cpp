#include <algorithm>
#include <core/logic/config_loader.h>
#include <core/unit/engine_context.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/logic/engine_core.h>
#include <core/manager/system_manager.h>
#include <ecs/logic/render_system.h>
#include <render/logic/post_process_pipeline.h>
#include <render/type/graphics_types.h>
#include <physics/interface/i_physics_world.h>
#include <platform/interface/i_window.h>
#include <core/logic/logger.h>

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
    else if (subCmd == "WINDOW_WIDTH")
    {
        int w = 800;
        ss >> w;
        config.width = w;
    }
    else if (subCmd == "WINDOW_HEIGHT")
    {
        int h = 600;
        ss >> h;
        config.height = h;
    }
    else if (subCmd == "WINDOW_MODE")
    {
        std::string modeStr;
        if (ss >> modeStr)
        {
            if (modeStr == "FULLSCREEN")
                config.windowMode = 1;
            else if (modeStr == "BORDERLESS")
                config.windowMode = 2;
            else if (modeStr == "BORDERLESS_FULLSCREEN" || modeStr == "STRETCH_FULLSCREEN")
                config.windowMode = 3;
            else if (modeStr == "WINDOWED")
                config.windowMode = 0;
            else
                config.windowMode = 0;
        }
    }
    else if (subCmd == "WINDOW_MONITOR")
    {
        int monitorIdx = 0;
        ss >> monitorIdx;
        config.monitorIndex = monitorIdx;
    }
    else if (subCmd == "WINDOW_REFRESH_RATE")
    {
        int rate = 0;
        ss >> rate;
        config.refreshRate = rate;
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
            if (modeStr == "FAST")
                config.physicsMode = 0;
            else if (modeStr == "BALANCED")
                config.physicsMode = 1;
            else if (modeStr == "ACCURATE")
                config.physicsMode = 2;
            else
            {
                try
                {
                    config.physicsMode = std::stoi(modeStr);
                }
                catch (...)
                {
                    config.physicsMode = 1;
                }
            }
        }
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            if (valStr == "FXAA")
                config.antialiasing = 1;
            else if (valStr == "TAA")
                config.antialiasing = 2;
            else
                config.antialiasing = 0;
        }
    }
    // Feature toggles
    else if (subCmd == "SHADOWS_ENABLED")
    {
        int enable = 1;
        ss >> enable;
        config.shadowsEnabled = (enable != 0);
    }
    else if (subCmd == "BLOOM_ENABLED")
    {
        int enable = 0;
        ss >> enable;
        config.bloomEnabled = (enable != 0);
    }
    else if (subCmd == "HDR_ENABLED")
    {
        int enable = 0;
        ss >> enable;
        config.hdrEnabled = (enable != 0);
    }
    else if (subCmd == "TAA_ENABLED")
    {
        int enable = 0;
        ss >> enable;
        config.taaEnabled = (enable != 0);
    }
    // Rendering parameters
    else if (subCmd == "GAMMA")
    {
        float val = 2.2f;
        ss >> val;
        config.gamma = val;
    }
    else if (subCmd == "EXPOSURE")
    {
        float val = 1.0f;
        ss >> val;
        config.exposure = val;
    }
    else if (subCmd == "BLOOM_INTENSITY")
    {
        float val = 1.0f;
        ss >> val;
        config.bloomIntensity = val;
    }
    else if (subCmd == "TAA_BLEND_FACTOR")
    {
        float val = 0.9f;
        ss >> val;
        config.taaBlendFactor = val;
    }
    else if (subCmd == "AMBIENT_INTENSITY")
    {
        float val = 0.1f;
        ss >> val;
        config.ambientIntensity = val;
    }
    else if (subCmd == "TONEMAPPING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            if (valStr == "NONE")
                config.tonemappingMode = 0;
            else if (valStr == "ACES")
                config.tonemappingMode = 1;
            else if (valStr == "REINHARD")
                config.tonemappingMode = 2;
            else
            {
                try { config.tonemappingMode = std::stoi(valStr); }
                catch (...) { config.tonemappingMode = 1; }
            }
        }
    }
    else if (subCmd == "RENDER_PATH")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            if (valStr == "FORWARD")
                config.renderPath = 0;
            else if (valStr == "DEFERRED")
                config.renderPath = 1;
            else
            {
                try { config.renderPath = std::stoi(valStr); }
                catch (...) { config.renderPath = 0; }
            }
        }
    }
    else if (subCmd == "CLEAR_COLOR")
    {
        float r = 0.1f, g = 0.1f, b = 0.1f, a = 1.0f;
        ss >> r >> g >> b >> a;
        config.clearColor[0] = r;
        config.clearColor[1] = g;
        config.clearColor[2] = b;
        config.clearColor[3] = a;
    }
    // Audio
    else if (subCmd == "VOLUME")
    {
        float val = 1.0f;
        ss >> val;
        config.masterVolume = val;
    }
    // Core
    else if (subCmd == "JOB_THREADS")
    {
        int val = -1;
        ss >> val;
        config.numJobThreads = val;
    }
    else if (subCmd == "TIME_SCALE")
    {
        float val = 1.0f;
        ss >> val;
        config.timeScale = val;
    }
    // Physics
    else if (subCmd == "GRAVITY")
    {
        float x = 0.0f, y = -9.81f, z = 0.0f;
        ss >> x >> y >> z;
        config.gravity[0] = x;
        config.gravity[1] = y;
        config.gravity[2] = z;
    }
    else if (subCmd == "MAX_SUB_STEPS")
    {
        int val = 10;
        ss >> val;
        config.maxSubSteps = val;
    }
    else if (subCmd == "PHYSICS_TICK_RATE")
    {
        float val = 60.0f;
        ss >> val;
        config.physicsTickRate = val;
    }
    else if (subCmd == "CCD")
    {
        int enable = 0;
        ss >> enable;
        config.ccdEnabled = (enable != 0);
    }
    else if (subCmd == "SOLVER_ITERATIONS")
    {
        int val = 10;
        ss >> val;
        config.solverIterations = val;
    }
    // Navigation
    else if (subCmd == "AGENT_RADIUS")
    {
        float val = 0.5f;
        ss >> val;
        config.agentRadius = val;
    }
    else if (subCmd == "AGENT_HEIGHT")
    {
        float val = 2.0f;
        ss >> val;
        config.agentHeight = val;
    }
    else if (subCmd == "WALKABLE_TAG")
    {
        std::string tag;
        if (ss >> tag)
            config.walkableTag = tag;
    }
    // Input
    else if (subCmd == "MOUSE_SENSITIVITY")
    {
        float val = 0.1f;
        ss >> val;
        // Store in a general place; scripts can read from config
    }
}

void ConfigLoader::LoadConfig(std::stringstream &ss, EngineContext ctx)
{

    std::string subCmd;

    ss >> subCmd;
    if (subCmd == "GRAPHICS_API")
    {
        std::string backend;
        if (ss >> backend && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.graphicsBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Graphics Backend: " << backend;
        }
    }
    else if (subCmd == "PHYSICS_ENGINE")
    {
        std::string backend;
        if (ss >> backend && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.physicsBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Physics Backend: " << backend;
        }
    }
    else if (subCmd == "AUDIO_ENGINE")
    {
        std::string backend;
        if (ss >> backend && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.audioBackend = backend;
            LOGGER_INFO("ConfigLoader") << "Scene requested Audio Backend: " << backend;
        }
    }
    else if (subCmd == "SHADOWS")
    {
        int mode = 1;
        ss >> mode;
        if (ctx.IsValid())
            ctx.systems->GetSystem<RenderSystem>()->SetShadowMode(mode);
    }
    else if (subCmd == "SHADOW_SIZE")
    {
        float size = 20.0f;
        ss >> size;
        if (ctx.IsValid())
            ctx.systems->GetSystem<RenderSystem>()->SetShadowProjectionSize(size);
    }

    else if (subCmd == "INSTANCING")
    {
        int enable = 0;
        ss >> enable;
        if (ctx.IsValid())
        {
            ctx.systems->GetSystem<RenderSystem>()->SetInstanceBatching(enable != 0);
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
            CullMode mode = CullMode::Back;
            if (modeStr == "FRONT")
                mode = CullMode::Front;
            else if (modeStr == "FRONT_AND_BACK")
                mode = CullMode::FrontAndBack;
            else if (modeStr != "BACK")
                LOGGER_WARN("ConfigLoader") << "Invalid CULL_FACE mode: " << modeStr << ". Supported: BACK, FRONT, FRONT_AND_BACK.";

            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetFaceCulling(true, mode);
        }
        else
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetFaceCulling(false);
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
            CompareFunc func = CompareFunc::Less;
            if (funcStr == "NEVER")
                func = CompareFunc::Never;
            else if (funcStr == "LESS")
                func = CompareFunc::Less;
            else if (funcStr == "EQUAL")
                func = CompareFunc::Equal;
            else if (funcStr == "LEQUAL")
                func = CompareFunc::Lequal;
            else if (funcStr == "GREATER")
                func = CompareFunc::Greater;
            else if (funcStr == "NOTEQUAL")
                func = CompareFunc::NotEqual;
            else if (funcStr == "GEQUAL")
                func = CompareFunc::Gequal;
            else if (funcStr == "ALWAYS")
                func = CompareFunc::Always;
            else
                LOGGER_WARN("ConfigLoader") << "Invalid DEPTH_TEST func: " << funcStr << ". Supported: NEVER, LESS, EQUAL, LEQUAL, GREATER, NOTEQUAL, GEQUAL, ALWAYS.";

            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetDepthTest(true, func);
        }
        else
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetDepthTest(false);
        }
    }
    else if (subCmd.find("WINDOW") != std::string::npos)
    {
    }
    else if (subCmd == "VSYNC")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (ctx.IsValid())
                ctx.io->GetMonitorManager().SetVsync(enable != 0);
        }
    }
    else if (subCmd == "FPS")
    {
        int fps = 0;
        if (ss >> fps)
        {
            if (ctx.IsValid())
                ctx.io->GetMonitorManager().SetFrameRateLimit(fps);
        }
    }
    else if (subCmd == "FRUSTUM")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetFrustumCulling(enable != 0);
        }
    }
    else if (subCmd == "RENDER_ORDER")
    {
        int enable = 0;
        if (ss >> enable && ctx.IsValid())
        {
            bool renderOrder = (enable != 0);
            ctx.systems->GetSystem<RenderSystem>()->SetRenderOrderEnabled(renderOrder);

            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.renderOrderEnabled = renderOrder;
        }
    }
    else if (subCmd == "FILTER_LAYER")
    {
        uint32_t mask = 0xFFFFFFFF;
        if (ss >> mask)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetFilterLayerMask(mask);
        }
    }
    else if (subCmd == "SHADOW_FRUSTUM")
    {
        int enable = 0;
        if (ss >> enable)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetShadowFrustumCulling(enable != 0);
        }
    }
    else if (subCmd == "SHADOW_DISTANCE")
    {
        float dist = 0.0f;
        if (ss >> dist)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetShadowDistanceCulling(dist);
        }
    }
    else if (subCmd == "DISTANCE")
    {
        float dist = 0.0f;
        if (ss >> dist)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetDistanceCulling(dist);
        }
    }
    else if (subCmd == "PHYSICS_MODE")
    {
        std::string modeStr;
        int mode = 1;
        if (ss >> modeStr)
        {
            if (modeStr == "FAST")
                mode = 0;
            else if (modeStr == "BALANCED")
                mode = 1;
            else if (modeStr == "ACCURATE")
                mode = 2;
            else
            {
                try
                {
                    mode = std::stoi(modeStr);
                }
                catch (...)
                {
                    LOGGER_WARN("ConfigLoader") << "Invalid PHYSICS_MODE: " << modeStr << ". Supported: FAST, BALANCED, ACCURATE (or 0, 1, 2).";
                }
            }

            if (ctx.IsValid())
            {
                ctx.physics->SetMode(mode);
            }
        }
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            AntiAliasingMode mode = AntiAliasingMode::NONE;
            if (valStr == "FXAA")
                mode = AntiAliasingMode::FXAA;
            else if (valStr == "TAA")
                mode = AntiAliasingMode::TAA;
            else if (valStr == "NONE")
                mode = AntiAliasingMode::NONE;
            else
                LOGGER_WARN("ConfigLoader") << "Invalid ANTIALIASING mode: " << valStr << ". Supported: NONE, FXAA, TAA.";

            if (ctx.IsValid())
            {
                ctx.systems->GetSystem<RenderSystem>()->SetAntiAliasingMode(mode);
            }
        }
    }
    // Feature toggles (runtime)
    else if (subCmd == "SHADOWS_ENABLED")
    {
        int enable = 1;
        if (ss >> enable && ctx.IsValid())
            ctx.systems->GetSystem<RenderSystem>()->SetEnableShadows(enable != 0);
    }
    else if (subCmd == "BLOOM_ENABLED")
    {
        int enable = 0;
        if (ss >> enable && ctx.IsValid())
            ctx.systems->GetPostProcess().SetBloomEnabled(enable != 0);
    }
    else if (subCmd == "HDR_ENABLED")
    {
        int enable = 0;
        if (ss >> enable && ctx.IsValid())
            ctx.systems->GetPostProcess().SetHDREnabled(enable != 0);
    }
    else if (subCmd == "TAA_ENABLED")
    {
        int enable = 0;
        if (ss >> enable && ctx.IsValid())
        {
            auto* rs = ctx.systems->GetSystem<RenderSystem>();
            rs->SetAntiAliasingMode(enable ? AntiAliasingMode::TAA : AntiAliasingMode::NONE);
        }
    }
    // Rendering parameters (runtime)
    else if (subCmd == "GAMMA")
    {
        float val = 2.2f;
        if (ss >> val && ctx.IsValid())
            ctx.systems->GetPostProcess().SetGamma(val);
    }
    else if (subCmd == "EXPOSURE")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
            ctx.systems->GetPostProcess().SetExposure(val);
    }
    else if (subCmd == "BLOOM_INTENSITY")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
            ctx.systems->GetPostProcess().SetBloomIntensity(val);
    }
    else if (subCmd == "TONEMAPPING")
    {
        std::string valStr;
        if (ss >> valStr && ctx.IsValid())
        {
            int mode = 1;
            if (valStr == "NONE") mode = 0;
            else if (valStr == "ACES") mode = 1;
            else if (valStr == "REINHARD") mode = 2;
            ctx.systems->GetPostProcess().SetTonemappingMode(mode);
        }
    }
    else if (subCmd == "RENDER_PATH")
    {
        std::string valStr;
        if (ss >> valStr && ctx.IsValid())
        {
            bool deferred = (valStr == "DEFERRED" || valStr == "1");
            ctx.systems->GetSystem<RenderSystem>()->SetDeferredRendering(deferred);
        }
    }
    else if (subCmd == "CLEAR_COLOR")
    {
        float r = 0.1f, g = 0.1f, b = 0.1f, a = 1.0f;
        ss >> r >> g >> b >> a;
        if (ctx.IsValid())
            ctx.systems->GetPostProcess().SetClearColor(r, g, b, a);
    }
    // Physics (runtime)
    else if (subCmd == "GRAVITY")
    {
        float x = 0.0f, y = -9.81f, z = 0.0f;
        ss >> x >> y >> z;
        if (ctx.IsValid())
            ctx.physics->SetGravity(glm::vec3(x, y, z));
    }
    else if (subCmd == "VOLUME")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.masterVolume = val;
        }
    }
    else if (subCmd == "TIME_SCALE")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.timeScale = val;
        }
    }
}
