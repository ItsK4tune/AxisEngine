#include <algorithm>
#include <core/logic/config_loader.h>
#include <core/logic/config_loader.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/app/runtime_core.h>
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


        try {
            size_t pos;
            int val = std::stoi(input, &pos);
            if (pos == input.length()) {
                for (auto const& [name, enumVal] : mapping) {
                    if (static_cast<int>(enumVal) == val) return enumVal;
                }
            }
        } catch (...) {}

        std::string clean = ToUpper(input);
        

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
    else if (subCmd == "SKYBOX_INTENSITY") {
        ss >> config.skyboxIntensity;
    }
    else if (subCmd == "AMBIENT_INTENSITY") {
        float val; ss >> val;

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
    else if (subCmd == "LIGHTING_MODE") {
        std::string val; ss >> val;
        config.lightingMode = ResolveEnum(val, {
            {"BAKE", LightingMode::Bake},
            {"LIGHT_PROBE", LightingMode::LightProbe},
            {"LIGHTPROBE", LightingMode::LightProbe},
            {"REFLECTION_PROBES", LightingMode::ReflectionProbes},
            {"REFLECTIONPROBES", LightingMode::ReflectionProbes},
            {"REAL_TIME", LightingMode::RealTime},
            {"REALTIME", LightingMode::RealTime}
        }, LightingMode::RealTime);
    }
}


