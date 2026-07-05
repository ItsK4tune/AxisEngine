#include <core/logic/config_serializer.h>
#include <core/logic/config_loader.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/logger.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>

static std::string FloatStr(float val)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6) << val;
    return ss.str();
}

static std::string FormatGravity(const float gravity[3])
{
    return FloatStr(gravity[0]) + " " + FloatStr(gravity[1]) + " " + FloatStr(gravity[2]);
}

bool ConfigSerializer::Deserialize(const std::string& filepath, AppConfig& config, bool headless)
{
    if (!std::filesystem::exists(filepath))
    {
        return false;
    }

    auto roots = YAMLParser::Parse(filepath);
    if (roots.empty())
    {
        return false;
    }

    for (const auto& root : roots)
    {
        if (root.key.rfind("axis_", 0) == 0 && root.key != "axis_config" && root.key != "axis_scene")
        {
            LOGGER_WARN("ConfigSerializer") << "Potential typo in root key: '" << root.key << "', expected 'axis_config' in " << filepath;
        }
    }

    std::vector<YAMLNode> activeRoots;
    if (roots.size() == 1 && roots[0].key == "axis_config")
    {
        activeRoots = roots[0].children;
    }
    else
    {
        bool found = false;
        for (const auto& root : roots)
        {
            if (root.key == "axis_config")
            {
                activeRoots = root.children;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    for (const auto& node : activeRoots)
    {
        std::stringstream ss;
        ss << node.key << " " << node.value;
        ConfigLoader::LoadConfig(ss, config, headless);
    }

    return true;
}

bool ConfigSerializer::Serialize(const std::string& filepath, const AppConfig& cfg)
{
    std::ofstream f(filepath);
    if (!f.is_open())
        return false;

    f << "axis_config:\n";

    auto SerialWriteKV = [](std::ofstream& out, int indent, const std::string& key, const std::string& value) {
        out << std::string(indent, ' ') << key << ": " << value << "\n";
    };

    // --- Core Settings ---
    if (!cfg.title.empty())
        SerialWriteKV(f, 4, "TITLE", cfg.title);
    
    auto LogLevelToStr = [](LogLevel level) {
        switch (level)
        {
            case LogLevel::None: return "NONE";
            case LogLevel::Minimal: return "MINIMAL";
            case LogLevel::Flex: return "FLEX";
            case LogLevel::Verbose: return "VERBOSE";
            case LogLevel::Debug: return "DEBUG";
            default: return "DEBUG";
        }
    };
    SerialWriteKV(f, 4, "LOG_LEVEL", LogLevelToStr(cfg.logLevel));
    SerialWriteKV(f, 4, "JOB_THREADS", std::to_string(cfg.numJobThreads));
    SerialWriteKV(f, 4, "TIME_SCALE", FloatStr(cfg.timeScale));
    if (!cfg.iconPath.empty())
        SerialWriteKV(f, 4, "ICON_PATH", cfg.iconPath);
    SerialWriteKV(f, 4, "HEADLESS", cfg.headlessMode ? "1" : "0");

    // --- Window Settings ---
    SerialWriteKV(f, 4, "WINDOW_WIDTH", std::to_string(cfg.window.width));
    SerialWriteKV(f, 4, "WINDOW_HEIGHT", std::to_string(cfg.window.height));

    auto WindowModeToStr = [](WindowMode mode) {
        switch (mode)
        {
            case WindowMode::Fullscreen:
                return "FULLSCREEN";
            case WindowMode::Borderless:
                return "BORDERLESS";
            case WindowMode::BorderlessFullscreen:
                return "BORDERLESS_FULLSCREEN";
            default:
                return "WINDOWED";
        }
    };
    SerialWriteKV(f, 4, "WINDOW_MODE", WindowModeToStr(cfg.window.windowMode));
    SerialWriteKV(f, 4, "VSYNC", cfg.window.vsync ? "1" : "0");
    SerialWriteKV(f, 4, "MONITOR", std::to_string(cfg.window.monitorIndex));
    SerialWriteKV(f, 4, "REFRESH_RATE", std::to_string(cfg.window.refreshRate));
    SerialWriteKV(f, 4, "FPS", std::to_string(cfg.window.frameRateLimit));

    // --- Graphics Settings ---
    auto GraphicsBackendToStr = [](GraphicsBackend backend) {
        switch (backend)
        {
            case GraphicsBackend::Vulkan: return "VULKAN";
            case GraphicsBackend::DirectX: return "DIRECTX";
            default: return "OPENGL";
        }
    };
    SerialWriteKV(f, 4, "GRAPHICS_API", GraphicsBackendToStr(cfg.graphics.graphicsBackend));
    SerialWriteKV(f, 4, "MSAA", std::to_string(cfg.graphics.msaaSamples));
    
    auto AntialiasingToStr = [](int aa) {
        switch (aa)
        {
            case 0: return "NONE";
            case 1: return "FXAA";
            case 2: return "TAA";
            default: return "NONE";
        }
    };
    SerialWriteKV(f, 4, "ANTIALIASING", AntialiasingToStr(cfg.graphics.antialiasing));
    SerialWriteKV(f, 4, "ANISOTROPY", FloatStr(cfg.graphics.maxAnisotropy));
    SerialWriteKV(f, 4, "RENDER_SCALE", FloatStr(cfg.graphics.renderScale));
    SerialWriteKV(f, 4, "ASYNC_RESOURCES", cfg.graphics.asyncResourceLoading ? "1" : "0");
    SerialWriteKV(f, 4, "STRICT_ASSET_LOADING", cfg.graphics.strictAssetLoading ? "1" : "0");

    // --- Render Settings ---
    SerialWriteKV(f, 4, "HDR_ENABLED", cfg.render.hdrEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "BLOOM_ENABLED", cfg.render.bloomEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "GAMMA", FloatStr(cfg.render.gamma));
    SerialWriteKV(f, 4, "EXPOSURE", FloatStr(cfg.render.exposure));
    SerialWriteKV(f, 4, "BLOOM_INTENSITY", FloatStr(cfg.render.bloomIntensity));
    SerialWriteKV(f, 4, "BLOOM_THRESHOLD", FloatStr(cfg.render.bloomThreshold));
    SerialWriteKV(f, 4, "BLOOM_RADIUS", FloatStr(cfg.render.bloomRadius));
    SerialWriteKV(f, 4, "SKYBOX_INTENSITY", FloatStr(cfg.render.skyboxIntensity));
    SerialWriteKV(f, 4, "AMBIENT_INTENSITY", FloatStr(cfg.render.ambientIntensity));
    SerialWriteKV(f, 4, "UI_REFERENCE_SIZE", FloatStr(cfg.render.uiReferenceWidth) + " " + FloatStr(cfg.render.uiReferenceHeight));

    auto TonemappingToStr = [](TonemappingMode mode) {
        switch (mode)
        {
            case TonemappingMode::ACES:
                return "ACES";
            case TonemappingMode::Reinhard:
                return "REINHARD";
            default:
                return "NONE";
        }
    };
    SerialWriteKV(f, 4, "TONEMAPPING", TonemappingToStr(cfg.render.tonemappingMode));
    
    std::string clrColor = FloatStr(cfg.render.clearColor[0]) + " " + 
                          FloatStr(cfg.render.clearColor[1]) + " " + 
                          FloatStr(cfg.render.clearColor[2]) + " " + 
                          FloatStr(cfg.render.clearColor[3]);
    SerialWriteKV(f, 4, "CLEAR_COLOR", clrColor);

    // --- Shadow Settings ---
    SerialWriteKV(f, 4, "SHADOWS_ENABLED", cfg.shadow.shadowsEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "SHADOWS", std::to_string(cfg.shadow.shadowMode));
    SerialWriteKV(f, 4, "SHADOW_RESOLUTION", std::to_string(cfg.shadow.shadowMapResolution));
    SerialWriteKV(f, 4, "SHADOW_SIZE", FloatStr(cfg.shadow.shadowProjectionSize));
    SerialWriteKV(f, 4, "SHADOW_FRUSTUM", cfg.shadow.shadowFrustumCullingEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "SHADOW_DISTANCE", FloatStr(cfg.shadow.shadowDistanceCulling));
    SerialWriteKV(f, 4, "SHADOW_BIAS", FloatStr(cfg.shadow.shadowBias));
    SerialWriteKV(f, 4, "SHADOW_SOFTNESS", std::to_string(cfg.shadow.shadowSoftness));

    // --- Physics Settings ---
    auto PhysicsBackendToStr = [](PhysicsBackend backend) {
        switch (backend)
        {
            case PhysicsBackend::PhysX: return "PHYSX";
            default: return "BULLET";
        }
    };
    SerialWriteKV(f, 4, "PHYSICS_ENGINE", PhysicsBackendToStr(cfg.physics.physicsBackend));
    
    auto PhysicsModeToStr = [](PhysicsMode mode) {
        switch (mode)
        {
            case PhysicsMode::Fast: return "FAST";
            case PhysicsMode::Accurate: return "ACCURATE";
            default: return "BALANCED";
        }
    };
    SerialWriteKV(f, 4, "PHYSICS_MODE", PhysicsModeToStr(cfg.physics.physicsMode));
    SerialWriteKV(f, 4, "GRAVITY", FormatGravity(cfg.physics.gravity));
    SerialWriteKV(f, 4, "MAX_SUBSTEPS", std::to_string(cfg.physics.maxSubSteps));
    SerialWriteKV(f, 4, "PHYSICS_TICKRATE", FloatStr(cfg.physics.physicsTickRate));
    SerialWriteKV(f, 4, "CCD_ENABLED", cfg.physics.ccdEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "CCD_THRESHOLD", FloatStr(cfg.physics.ccdThreshold));
    SerialWriteKV(f, 4, "SOLVER_ITERATIONS", std::to_string(cfg.physics.solverIterations));

    // --- Input Settings ---
    SerialWriteKV(f, 4, "MOUSE_SENSITIVITY_X", FloatStr(cfg.input.mouseSensitivityX));
    SerialWriteKV(f, 4, "MOUSE_SENSITIVITY_Y", FloatStr(cfg.input.mouseSensitivityY));
    SerialWriteKV(f, 4, "MOUSE_INVERT_X", cfg.input.mouseInvertX ? "1" : "0");
    SerialWriteKV(f, 4, "MOUSE_INVERT_Y", cfg.input.mouseInvertY ? "1" : "0");
    SerialWriteKV(f, 4, "RAW_MOUSE_INPUT", cfg.input.rawMouseInput ? "1" : "0");

    // --- Audio Settings ---
    auto AudioBackendToStr = [](AudioBackend backend) {
        switch (backend)
        {
            case AudioBackend::IrrKlang: return "IRRKLANG";
            case AudioBackend::FMOD: return "FMOD";
            case AudioBackend::OpenAL: return "OPENAL";
            default: return "NULL";
        }
    };
    SerialWriteKV(f, 4, "AUDIO_ENGINE", AudioBackendToStr(cfg.audio.audioBackend));
    SerialWriteKV(f, 4, "VOLUME", FloatStr(cfg.audio.masterVolume));
    if (!cfg.audio.audioDevice.empty())
        SerialWriteKV(f, 4, "AUDIO_DEVICE", cfg.audio.audioDevice);

    // --- Culling Settings ---
    SerialWriteKV(f, 4, "CULL_FACE", cfg.culling.cullFaceEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "DEPTH_TEST", cfg.culling.depthTestEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "FRUSTUM", cfg.culling.frustumCullingEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "OCCLUSION_CULLING", cfg.culling.occlusionCullingEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "INSTANCING", cfg.culling.instanceBatchingEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "RENDER_ORDER", cfg.culling.renderOrderEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "FILTER_LAYER", std::to_string(cfg.culling.filterLayerMask));
    SerialWriteKV(f, 4, "DISTANCE", FloatStr(cfg.culling.distanceCulling));

    // --- Debug Settings ---
    SerialWriteKV(f, 4, "WIREFRAME_MODE", cfg.debug.wireframeMode ? "1" : "0");
    SerialWriteKV(f, 4, "NO_TEXTURE", cfg.debug.noTexture ? "1" : "0");
    SerialWriteKV(f, 4, "PHYSICS_DEBUG", cfg.debug.physicsDebug ? "1" : "0");
    SerialWriteKV(f, 4, "UI_ENABLED", cfg.debug.uiEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "GIZMOS", cfg.debug.gizmos ? "1" : "0");
    SerialWriteKV(f, 4, "LIGHT_GIZMOS", cfg.debug.lightGizmos ? "1" : "0");
    SerialWriteKV(f, 4, "ENTITY_NAMES", cfg.debug.entityNames ? "1" : "0");
    SerialWriteKV(f, 4, "AUDIO_DEBUG", cfg.debug.audioDebug ? "1" : "0");
    SerialWriteKV(f, 4, "PARTICLE_DEBUG", cfg.debug.particleDebug ? "1" : "0");
    SerialWriteKV(f, 4, "GRID_SNAP_ENABLED", cfg.debug.gridSnapEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "GRID_INDICATOR_ENABLED", cfg.debug.gridIndicatorEnabled ? "1" : "0");
    SerialWriteKV(f, 4, "GRID_SNAP_TRANSLATION", FloatStr(cfg.debug.gridSnapTranslation));
    SerialWriteKV(f, 4, "GRID_SNAP_ROTATION", FloatStr(cfg.debug.gridSnapRotation));
    SerialWriteKV(f, 4, "GRID_SNAP_SCALE", FloatStr(cfg.debug.gridSnapScale));

    auto LightingModeToStr = [](LightingMode mode) {
        switch (mode)
        {
            case LightingMode::Bake:
                return "BAKE";
            case LightingMode::LightProbe:
                return "LIGHT_PROBE";
            case LightingMode::ReflectionProbes:
                return "REFLECTION_PROBES";
            default:
                return "REAL_TIME";
        }
    };
    SerialWriteKV(f, 4, "LIGHTING_MODE", LightingModeToStr(cfg.lightingMode));

    return true;
}
