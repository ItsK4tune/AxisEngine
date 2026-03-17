#include <algorithm>
#include <core/io/config_loader.h>
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

#include <unordered_map>

namespace {
    std::string ToUpper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    template<typename T>
    T ResolveEnum(const std::string& input, const std::unordered_map<std::string, T>& mapping, T defaultValue) {
        if (input.empty()) return defaultValue;

        // Try numeric
        try {
            size_t pos;
            int val = std::stoi(input, &pos);
            if (pos == input.length()) { // Full numeric string
                for (auto const& [name, enumVal] : mapping) {
                    if (static_cast<int>(enumVal) == val) return enumVal;
                }
            }
        } catch (...) {}

        std::string clean = ToUpper(input);
        
        // Handle C++ style prefixes if present: "TonemappingMode::ACES" -> "ACES"
        size_t colon = clean.rfind("::");
        if (colon != std::string::npos) {
            clean = clean.substr(colon + 2);
        }

        auto it = mapping.find(clean);
        if (it != mapping.end()) return it->second;

        return defaultValue;
    }
}

void ConfigLoader::LoadConfig(std::stringstream &ss, AppConfig &config)
{
    std::string subCmd;
    ss >> subCmd;

    if (subCmd == "GRAPHICS_API") {
        std::string val; ss >> val;
        config.graphicsBackend = ResolveEnum(val, {
            {"OPENGL", GraphicsBackend::OpenGL},
            {"VULKAN", GraphicsBackend::Vulkan},
            {"DIRECTX", GraphicsBackend::DirectX}
        }, GraphicsBackend::OpenGL);
    }
    else if (subCmd == "PHYSICS_ENGINE") {
        std::string val; ss >> val;
        config.physicsBackend = ResolveEnum(val, {
            {"BULLET", PhysicsBackend::Bullet},
            {"PHYSX", PhysicsBackend::PhysX}
        }, PhysicsBackend::Bullet);
    }
    else if (subCmd == "AUDIO_ENGINE") {
        std::string val; ss >> val;
        config.audioBackend = ResolveEnum(val, {
            {"IRRKLANG", AudioBackend::IrrKlang},
            {"FMOD", AudioBackend::FMOD},
            {"OPENAL", AudioBackend::OpenAL}
        }, AudioBackend::IrrKlang);
    }
    else if (subCmd == "TONEMAPPING") {
        std::string val; ss >> val;
        config.tonemappingMode = ResolveEnum(val, {
            {"NONE", TonemappingMode::None},
            {"ACES", TonemappingMode::ACES},
            {"REINHARD", TonemappingMode::Reinhard}
        }, TonemappingMode::ACES);
    }
    else if (subCmd == "RENDER_PATH") {
        std::string val; ss >> val;
        config.renderPath = ResolveEnum(val, {
            {"FORWARD", RenderPath::Forward},
            {"DEFERRED", RenderPath::Deferred}
        }, RenderPath::Forward);
    }
    else if (subCmd == "LOG_LEVEL") {
        std::string val; ss >> val;
        config.logLevel = ResolveEnum(val, {
            {"NONE", LogLevel::None},
            {"MINIMAL", LogLevel::Minimal},
            {"FLEX", LogLevel::Flex},
            {"VERBOSE", LogLevel::Verbose},
            {"DEBUG", LogLevel::Debug}
        }, LogLevel::Debug);
    }
    else if (subCmd == "PHYSICS_MODE") {
        std::string val; ss >> val;
        config.physicsMode = ResolveEnum(val, {
            {"FAST", PhysicsMode::Fast},
            {"BALANCED", PhysicsMode::Balanced},
            {"ACCURATE", PhysicsMode::Accurate}
        }, PhysicsMode::Balanced);
    }
    else if (subCmd == "SHADOWS") {
        int mode; ss >> mode;
        config.shadowMode = mode;
    }
    else if (subCmd == "SHADOW_SIZE") {
        ss >> config.shadowProjectionSize;
    }
    else if (subCmd == "SHADOW_RESOLUTION") {
        ss >> config.shadowMapResolution;
    }
    else if (subCmd == "INSTANCING") {
        int enable; ss >> enable;
        config.instanceBatchingEnabled = (enable != 0);
    }
    else if (subCmd == "CULL_FACE") {
        int enable; ss >> enable;
        config.cullFaceEnabled = (enable != 0);
    }
    else if (subCmd == "DEPTH_TEST") {
        int enable; ss >> enable;
        config.depthTestEnabled = (enable != 0);
    }
    else if (subCmd == "WINDOW_WIDTH") {
        ss >> config.width;
    }
    else if (subCmd == "WINDOW_HEIGHT") {
        ss >> config.height;
    }
    else if (subCmd == "WINDOW_MODE") {
        std::string modeStr; ss >> modeStr;
        config.windowMode = ResolveEnum(modeStr, {
            {"WINDOWED", WindowMode::Windowed},
            {"FULLSCREEN", WindowMode::Fullscreen},
            {"BORDERLESS", WindowMode::Borderless},
            {"BORDERLESS_FULLSCREEN", WindowMode::BorderlessFullscreen}
        }, WindowMode::Windowed);
    }
    else if (subCmd == "VSYNC") {
        int enable; ss >> enable;
        config.vsync = (enable != 0);
    }
    else if (subCmd == "MONITOR") {
        ss >> config.monitorIndex;
    }
    else if (subCmd == "REFRESH_RATE") {
        ss >> config.refreshRate;
    }
    else if (subCmd == "FPS") {
        ss >> config.frameRateLimit;
    }
    else if (subCmd == "FRUSTUM") {
        int enable; ss >> enable;
        config.frustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "SHADOW_FRUSTUM") {
        int enable; ss >> enable;
        config.shadowFrustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "SHADOW_DISTANCE") {
        ss >> config.shadowDistanceCulling;
    }
    else if (subCmd == "SHADOW_BIAS") {
        ss >> config.shadowBias;
    }
    else if (subCmd == "SHADOW_SOFTNESS") {
        ss >> config.shadowSoftness;
    }
    else if (subCmd == "DISTANCE") {
        ss >> config.distanceCulling;
    }
    else if (subCmd == "MOUSE_SENSITIVITY") {
        ss >> config.mouseSensitivityX;
        config.mouseSensitivityY = config.mouseSensitivityX;
    }
    else if (subCmd == "MOUSE_SENSITIVITY_X") {
        ss >> config.mouseSensitivityX;
    }
    else if (subCmd == "MOUSE_SENSITIVITY_Y") {
        ss >> config.mouseSensitivityY;
    }
    else if (subCmd == "MOUSE_INVERT_X") {
        int invert; ss >> invert;
        config.mouseInvertX = (invert != 0);
    }
    else if (subCmd == "MOUSE_INVERT_Y") {
        int invert; ss >> invert;
        config.mouseInvertY = (invert != 0);
    }
    else if (subCmd == "MSAA") {
        ss >> config.msaaSamples;
    }
    else if (subCmd == "ANISOTROPY") {
        ss >> config.maxAnisotropy;
    }
    else if (subCmd == "RENDER_SCALE") {
        ss >> config.renderScale;
    }
    else if (subCmd == "ASYNC_RESOURCES") {
        int enable; ss >> enable;
        config.asyncResourceLoading = (enable != 0);
    }
    else if (subCmd == "SHADOWS_ENABLED") {
        int enable; ss >> enable;
        config.shadowsEnabled = (enable != 0);
    }
    else if (subCmd == "BLOOM_ENABLED") {
        int enable; ss >> enable;
        config.bloomEnabled = (enable != 0);
    }
    else if (subCmd == "HDR_ENABLED") {
        int enable; ss >> enable;
        config.hdrEnabled = (enable != 0);
    }
    else if (subCmd == "GAMMA") {
        ss >> config.gamma;
    }
    else if (subCmd == "EXPOSURE") {
        ss >> config.exposure;
    }
    else if (subCmd == "BLOOM_INTENSITY") {
        ss >> config.bloomIntensity;
    }
    else if (subCmd == "BLOOM_THRESHOLD") {
        ss >> config.bloomThreshold;
    }
    else if (subCmd == "BLOOM_RADIUS") {
        ss >> config.bloomRadius;
    }
    else if (subCmd == "SKYBOX_INTENSITY") {
        ss >> config.skyboxIntensity;
    }
    else if (subCmd == "FOG_ENABLED") {
        int enable; ss >> enable;
        config.fogEnabled = (enable != 0);
    }
    else if (subCmd == "FOG_COLOR") {
        ss >> config.fogColor[0] >> config.fogColor[1] >> config.fogColor[2];
    }
    else if (subCmd == "FOG_DENSITY") {
        ss >> config.fogDensity;
    }
    else if (subCmd == "AMBIENT_INTENSITY") {
        float val; ss >> val;
        // unused
    }
    else if (subCmd == "VOLUME") {
        ss >> config.masterVolume;
    }
    else if (subCmd == "JOB_THREADS") {
        ss >> config.numJobThreads;
    }
    else if (subCmd == "TIME_SCALE") {
        ss >> config.timeScale;
    }
    else if (subCmd == "GRAVITY") {
        ss >> config.gravity[0] >> config.gravity[1] >> config.gravity[2];
    }
    else if (subCmd == "MAX_SUBSTEPS") {
        ss >> config.maxSubSteps;
    }
    else if (subCmd == "PHYSICS_TICKRATE") {
        ss >> config.physicsTickRate;
    }
    else if (subCmd == "CCD_ENABLED") {
        int enable; ss >> enable;
        config.ccdEnabled = (enable != 0);
    }
    else if (subCmd == "CCD_THRESHOLD") {
        ss >> config.ccdThreshold;
    }
    else if (subCmd == "SOLVER_ITERATIONS") {
        ss >> config.solverIterations;
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
            cfg.graphicsBackend = ResolveEnum(backend, {
                {"OPENGL", GraphicsBackend::OpenGL},
                {"VULKAN", GraphicsBackend::Vulkan},
                {"DIRECTX", GraphicsBackend::DirectX}
            }, GraphicsBackend::OpenGL);
            LOGGER_INFO("ConfigLoader") << "Scene requested Graphics Backend: " << backend;
        }
    }
    else if (subCmd == "PHYSICS_ENGINE")
    {
        std::string backend;
        if (ss >> backend && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.physicsBackend = ResolveEnum(backend, {
                {"BULLET", PhysicsBackend::Bullet},
                {"PHYSX", PhysicsBackend::PhysX}
            }, PhysicsBackend::Bullet);
            LOGGER_INFO("ConfigLoader") << "Scene requested Physics Backend: " << backend;
        }
    }
    else if (subCmd == "AUDIO_ENGINE")
    {
        std::string backend;
        if (ss >> backend && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.audioBackend = ResolveEnum(backend, {
                {"IRRKLANG", AudioBackend::IrrKlang},
                {"FMOD", AudioBackend::FMOD},
                {"OPENAL", AudioBackend::OpenAL}
            }, AudioBackend::IrrKlang);
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
    else if (subCmd == "DISTANCE")
    {
        float dist = 0.0f;
        if (ss >> dist)
        {
            if (ctx.IsValid())
                ctx.systems->GetSystem<RenderSystem>()->SetDistanceCulling(dist);
        }
    }
    else if (subCmd == "MAX_SUBSTEPS")
    {
        int steps = 10;
        if (ss >> steps && ctx.IsValid())
            ctx.runtime->GetEngineLoop().SetMaxSubSteps(steps);
    }
    else if (subCmd == "PHYSICS_TICKRATE")
    {
        float rate = 60.0f;
        if (ss >> rate && ctx.IsValid())
            ctx.runtime->GetEngineLoop().SetPhysicsStep(1.0f / rate);
    }
    else if (subCmd == "PHYSICS_MODE")
    {
        std::string modeStr;
        if (ss >> modeStr)
        {
            PhysicsMode mode = ResolveEnum(modeStr, {
                {"FAST", PhysicsMode::Fast},
                {"BALANCED", PhysicsMode::Balanced},
                {"ACCURATE", PhysicsMode::Accurate}
            }, PhysicsMode::Balanced);

            if (ctx.IsValid())
            {
                ctx.physics->SetMode(static_cast<int>(mode));
                AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
                cfg.physicsMode = mode;
            }
        }
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string valStr;
        if (ss >> valStr)
        {
            AntiAliasingMode mode = ResolveEnum(valStr, {
                {"NONE", AntiAliasingMode::NONE},
                {"FXAA", AntiAliasingMode::FXAA},
                {"TAA", AntiAliasingMode::TAA}
            }, AntiAliasingMode::FXAA);

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
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.bloomEnabled = (enable != 0);
        }
    }
    else if (subCmd == "HDR_ENABLED")
    {
        int enable = 0;
        if (ss >> enable && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.hdrEnabled = (enable != 0);
        }
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
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.gamma = val;
        }
    }
    else if (subCmd == "EXPOSURE")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.exposure = val;
        }
    }
    else if (subCmd == "BLOOM_INTENSITY")
    {
        float val = 1.0f;
        if (ss >> val && ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.bloomIntensity = val;
        }
    }
    else if (subCmd == "TONEMAPPING")
    {
        std::string valStr;
        if (ss >> valStr && ctx.IsValid())
        {
            TonemappingMode mode = ResolveEnum(valStr, {
                {"NONE", TonemappingMode::None},
                {"ACES", TonemappingMode::ACES},
                {"REINHARD", TonemappingMode::Reinhard}
            }, TonemappingMode::ACES);
            
            if (ctx.IsValid())
            {
                AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
                cfg.tonemappingMode = mode;
            }
        }
    }
    else if (subCmd == "RENDER_PATH")
    {
        std::string valStr;
        if (ss >> valStr && ctx.IsValid())
        {
            RenderPath path = ResolveEnum(valStr, {
                {"FORWARD", RenderPath::Forward},
                {"DEFERRED", RenderPath::Deferred}
            }, RenderPath::Forward);
            ctx.systems->GetSystem<RenderSystem>()->SetDeferredRendering(path == RenderPath::Deferred);
            
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.renderPath = path;
        }
    }
    else if (subCmd == "CLEAR_COLOR")
    {
        float r = 0.1f, g = 0.1f, b = 0.1f, a = 1.0f;
        ss >> r >> g >> b >> a;
        if (ctx.IsValid())
        {
            AppConfig &cfg = const_cast<AppConfig &>(ctx.runtime->GetConfig());
            cfg.clearColor[0] = r; cfg.clearColor[1] = g; cfg.clearColor[2] = b; cfg.clearColor[3] = a;
        }
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
