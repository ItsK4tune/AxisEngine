#include <core/logic/config_loader.h>
#include <core/logic/logger.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <unordered_map>

namespace
{
std::string ToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char value) { return static_cast<char>(std::toupper(value)); });
    return s;
}

std::string ReadRemaining(std::stringstream& stream)
{
    std::string value;
    std::getline(stream, value);
    const size_t first = value.find_first_not_of(" \t");
    return first == std::string::npos ? std::string{} : value.substr(first);
}

template <typename T>
T ResolveEnum(const char* key, const std::string& input, const std::unordered_map<std::string, T>& mapping,
              T currentValue)
{
    if (!input.empty())
    {
        int val = 0;
        const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), val);
        if (error == std::errc{} && end == input.data() + input.size())
        {
            for (const auto& [name, enumValue] : mapping)
            {
                if (static_cast<int>(enumValue) == val)
                    return enumValue;
            }
        }
        else
        {
            std::string clean = ToUpper(input);
            const size_t colon = clean.rfind("::");
            if (colon != std::string::npos)
                clean = clean.substr(colon + 2);

            if (const auto it = mapping.find(clean); it != mapping.end())
                return it->second;
        }
    }

    LOGGER_WARN("ConfigLoader") << "Invalid value '" << input << "' for " << key << "; keeping current value.";
    return currentValue;
}

bool LoadOptimizationSetting(const std::string& key, std::stringstream& stream, OptimizationConfig& config)
{
    static const std::unordered_map<std::string, bool OptimizationConfig::*> boolFields = {
        {"OPT_RESOURCE_HOT_RELOAD", &OptimizationConfig::resourceHotReloadEnabled},
        {"OPT_RESOURCE_UPLOAD_BUDGET", &OptimizationConfig::resourceUploadBudgetEnabled},
        {"OPT_DISCARD_CPU_MESH_DATA_AFTER_UPLOAD", &OptimizationConfig::discardCpuMeshDataAfterUpload},
        {"OPT_COMPRESSED_TEXTURE_LOADING", &OptimizationConfig::compressedTextureLoadingEnabled},
        {"OPT_STREAMING_UPDATE_THROTTLING", &OptimizationConfig::streamingUpdateThrottlingEnabled},
        {"OPT_REFLECTION_CAPTURE_BUDGET", &OptimizationConfig::reflectionCaptureBudgetEnabled},
        {"OPT_SHADOW_PARALLEL_BUILD", &OptimizationConfig::shadowParallelBuildEnabled},
        {"OPT_ANIMATION_PARALLEL_EVALUATION", &OptimizationConfig::animationParallelEvaluationEnabled},
        {"OPT_NAVIGATION_SPATIAL_HASH", &OptimizationConfig::navigationSpatialHashEnabled},
        {"OPT_NAVIGATION_ASYNC_PATHFINDING", &OptimizationConfig::navigationAsyncPathfindingEnabled},
        {"OPT_NAVMESH_REBUILD_BUDGET", &OptimizationConfig::navMeshRebuildBudgetEnabled},
        {"OPT_NAVIGATION_DIRTY_TILES", &OptimizationConfig::navigationDirtyTilesEnabled},
        {"OPT_NETWORK_BATCHING", &OptimizationConfig::networkBatchingEnabled},
        {"OPT_NETWORK_REPLICATION", &OptimizationConfig::networkReplicationEnabled},
        {"OPT_PARTICLE_SPAWN_BUDGET", &OptimizationConfig::particleSpawnBudgetEnabled},
        {"OPT_PARTICLE_BATCHING", &OptimizationConfig::particleBatchingEnabled},
        {"OPT_RENDER_STATE_CACHE", &OptimizationConfig::renderStateCacheEnabled},
        {"OPT_PERSISTENT_MAPPED_BUFFERS", &OptimizationConfig::persistentMappedBuffersEnabled},
        {"OPT_TILED_LIGHT_CULLING", &OptimizationConfig::tiledLightCullingEnabled},
        {"OPT_GBUFFER_ENTITY_ID", &OptimizationConfig::gbufferEntityIdEnabled},
        {"OPT_PHYSICS_MESH_SHAPE_CACHE", &OptimizationConfig::physicsMeshShapeCacheEnabled},
        {"OPT_UI_LAYOUT_CACHE", &OptimizationConfig::uiLayoutCacheEnabled},
        {"OPT_VIDEO_ASYNC_DECODE", &OptimizationConfig::videoAsyncDecodeEnabled},
    };
    static const std::unordered_map<std::string, int OptimizationConfig::*> intFields = {
        {"OPT_MAX_MODEL_UPLOADS_PER_FRAME", &OptimizationConfig::maxModelUploadsPerFrame},
        {"OPT_MAX_TEXTURE_UPLOADS_PER_FRAME", &OptimizationConfig::maxTextureUploadsPerFrame},
        {"OPT_MAX_REFLECTION_PROBE_FACES_PER_FRAME", &OptimizationConfig::maxReflectionProbeFacesPerFrame},
        {"OPT_MAX_PLANAR_REFLECTION_CAPTURES_PER_FRAME", &OptimizationConfig::maxPlanarReflectionCapturesPerFrame},
        {"OPT_SHADOW_PARALLEL_THRESHOLD", &OptimizationConfig::shadowParallelThreshold},
        {"OPT_ANIMATION_PARALLEL_THRESHOLD", &OptimizationConfig::animationParallelThreshold},
        {"OPT_NAVIGATION_MAX_PATH_REQUESTS_PER_FRAME", &OptimizationConfig::navigationMaxPathRequestsPerFrame},
        {"OPT_MAX_NAVMESH_REBUILDS_PER_FRAME", &OptimizationConfig::maxNavMeshRebuildsPerFrame},
        {"OPT_NAVIGATION_MAX_DIRTY_TILES_PER_FRAME", &OptimizationConfig::navigationMaxDirtyTilesPerFrame},
        {"OPT_NETWORK_MAX_EVENTS_PER_UPDATE", &OptimizationConfig::networkMaxEventsPerUpdate},
        {"OPT_NETWORK_MAX_BYTES_PER_UPDATE", &OptimizationConfig::networkMaxBytesPerUpdate},
        {"OPT_PARTICLE_MAX_SPAWN_PER_FRAME", &OptimizationConfig::particleMaxSpawnPerFrame},
        {"OPT_TILED_LIGHT_TILE_SIZE", &OptimizationConfig::tiledLightTileSize},
    };
    static const std::unordered_map<std::string, float OptimizationConfig::*> floatFields = {
        {"OPT_STREAMING_CHECK_INTERVAL", &OptimizationConfig::streamingCheckIntervalSeconds},
        {"OPT_NAVIGATION_AGENT_CELL_SIZE", &OptimizationConfig::navigationAgentCellSize},
        {"OPT_NAVIGATION_NAVMESH_TILE_SIZE", &OptimizationConfig::navigationNavMeshTileSize},
        {"OPT_NETWORK_MAX_EVENT_PROCESSING_MS", &OptimizationConfig::networkMaxEventProcessingMs},
        {"OPT_NETWORK_REPLICATION_RATE_HZ", &OptimizationConfig::networkReplicationRateHz},
        {"OPT_NETWORK_INTEREST_RADIUS", &OptimizationConfig::networkInterestRadius},
    };

    if (const auto field = boolFields.find(key); field != boolFields.end())
    {
        int value = 0;
        stream >> value;
        config.*(field->second) = value != 0;
        return true;
    }
    if (const auto field = intFields.find(key); field != intFields.end())
    {
        stream >> config.*(field->second);
        return true;
    }
    if (const auto field = floatFields.find(key); field != floatFields.end())
    {
        stream >> config.*(field->second);
        return true;
    }
    return false;
}
}  // namespace

void ConfigLoader::LoadConfig(std::stringstream& ss, AppConfig& config, bool headless)
{
    std::string subCmd;
    ss >> subCmd;

    // In headless mode, only load core + physics config
    if (headless)
    {
        static const std::unordered_map<std::string, bool> s_HeadlessCritical = {
            {"LOG_LEVEL", true},        {"JOB_THREADS", true},  {"TIME_SCALE", true},    {"HEADLESS", true},
            {"LOAD_DEFAULT_ASSETS", true}, {"DEFAULT_ASSET_MANIFEST", true},
            {"PHYSICS_ENGINE", true},   {"PHYSICS_MODE", true}, {"GRAVITY", true},       {"MAX_SUBSTEPS", true},
            {"PHYSICS_TICKRATE", true}, {"CCD_ENABLED", true},  {"CCD_THRESHOLD", true}, {"SOLVER_ITERATIONS", true},
            {"OPT_RESOURCE_UPLOAD_BUDGET", true}, {"OPT_MAX_MODEL_UPLOADS_PER_FRAME", true},
            {"OPT_MAX_TEXTURE_UPLOADS_PER_FRAME", true}, {"OPT_STREAMING_UPDATE_THROTTLING", true},
            {"OPT_STREAMING_CHECK_INTERVAL", true}, {"OPT_REFLECTION_CAPTURE_BUDGET", true},
            {"OPT_MAX_REFLECTION_PROBE_FACES_PER_FRAME", true},
            {"OPT_MAX_PLANAR_REFLECTION_CAPTURES_PER_FRAME", true}, {"OPT_SHADOW_PARALLEL_BUILD", true},
            {"OPT_SHADOW_PARALLEL_THRESHOLD", true}, {"OPT_ANIMATION_PARALLEL_EVALUATION", true},
            {"OPT_ANIMATION_PARALLEL_THRESHOLD", true}, {"OPT_NAVIGATION_SPATIAL_HASH", true},
            {"OPT_NAVIGATION_AGENT_CELL_SIZE", true}, {"OPT_NETWORK_BATCHING", true},
            {"OPT_NETWORK_MAX_EVENTS_PER_UPDATE", true}, {"OPT_NETWORK_MAX_EVENT_PROCESSING_MS", true},
            {"OPT_PARTICLE_SPAWN_BUDGET", true}, {"OPT_PARTICLE_MAX_SPAWN_PER_FRAME", true}};
        // Dedicated/headless builds still run streaming, navigation, networking,
        // physics and resource jobs. Keep every optimization policy loadable so
        // adding a new OPT_* key cannot silently diverge from desktop builds.
        const bool optimizationSetting = subCmd.rfind("OPT_", 0) == 0;
        if (!optimizationSetting && s_HeadlessCritical.find(subCmd) == s_HeadlessCritical.end())
            return;  // skip non-critical config in headless
    }

    if (subCmd == "GRAPHICS_API")
    {
        std::string val;
        ss >> val;
        config.graphics.graphicsBackend = ResolveEnum("GRAPHICS_API", val,
                                                      {{"OPENGL", GraphicsBackend::OpenGL},
                                                       {"VULKAN", GraphicsBackend::Vulkan},
                                                       {"DIRECTX", GraphicsBackend::DirectX}},
                                                      config.graphics.graphicsBackend);
    }
    else if (subCmd == "PHYSICS_ENGINE")
    {
        std::string val;
        ss >> val;
        config.physics.physicsBackend =
            ResolveEnum("PHYSICS_ENGINE", val, {{"BULLET", PhysicsBackend::Bullet}, {"PHYSX", PhysicsBackend::PhysX}},
                        config.physics.physicsBackend);
    }
    else if (subCmd == "AUDIO_ENGINE")
    {
        std::string val;
        ss >> val;
        config.audio.audioBackend = ResolveEnum("AUDIO_ENGINE", val,
                                                {{"NULL", AudioBackend::Null},
                                                 {"NONE", AudioBackend::Null},
                                                 {"IRRKLANG", AudioBackend::IrrKlang},
                                                 {"FMOD", AudioBackend::FMOD},
                                                 {"OPENAL", AudioBackend::OpenAL}},
                                                config.audio.audioBackend);
    }
    else if (subCmd == "TONEMAPPING")
    {
        std::string val;
        ss >> val;
        config.render.tonemappingMode = ResolveEnum(
            "TONEMAPPING", val,
            {{"NONE", TonemappingMode::None}, {"ACES", TonemappingMode::ACES}, {"REINHARD", TonemappingMode::Reinhard}},
            config.render.tonemappingMode);
    }
    else if (subCmd == "LOG_LEVEL")
    {
        std::string val;
        ss >> val;
        config.logLevel = ResolveEnum("LOG_LEVEL", val,
                                      {{"NONE", LogLevel::None},
                                       {"MINIMAL", LogLevel::Minimal},
                                       {"FLEX", LogLevel::Flex},
                                       {"VERBOSE", LogLevel::Verbose},
                                       {"DEBUG", LogLevel::Debug}},
                                      config.logLevel);
    }
    else if (subCmd == "PHYSICS_MODE")
    {
        std::string val;
        ss >> val;
        config.physics.physicsMode = ResolveEnum(
            "PHYSICS_MODE", val,
            {{"FAST", PhysicsMode::Fast}, {"BALANCED", PhysicsMode::Balanced}, {"ACCURATE", PhysicsMode::Accurate}},
            config.physics.physicsMode);
    }
    else if (subCmd == "SHADOWS")
    {
        int mode;
        ss >> mode;
        config.shadow.shadowMode = mode;
    }
    else if (subCmd == "SHADOW_SIZE")
    {
        ss >> config.shadow.shadowProjectionSize;
    }
    else if (subCmd == "SHADOW_RESOLUTION")
    {
        ss >> config.shadow.shadowMapResolution;
    }
    else if (subCmd == "INSTANCING")
    {
        int enable;
        ss >> enable;
        config.culling.instanceBatchingEnabled = (enable != 0);
    }
    else if (subCmd == "CULL_FACE")
    {
        int enable;
        ss >> enable;
        config.culling.cullFaceEnabled = (enable != 0);
    }
    else if (subCmd == "DEPTH_TEST")
    {
        int enable;
        ss >> enable;
        config.culling.depthTestEnabled = (enable != 0);
    }
    else if (subCmd == "STENCIL_TEST")
    {
        int enable;
        ss >> enable;
        config.culling.stencilTestEnabled = (enable != 0);
    }
    else if (subCmd == "WINDOW_WIDTH")
    {
        ss >> config.window.width;
    }
    else if (subCmd == "WINDOW_HEIGHT")
    {
        ss >> config.window.height;
    }
    else if (subCmd == "WINDOW_MODE")
    {
        std::string modeStr;
        ss >> modeStr;
        config.window.windowMode = ResolveEnum("WINDOW_MODE", modeStr,
                                               {{"WINDOWED", WindowMode::Windowed},
                                                {"FULLSCREEN", WindowMode::Fullscreen},
                                                {"BORDERLESS", WindowMode::Borderless},
                                                {"BORDERLESS_FULLSCREEN", WindowMode::BorderlessFullscreen}},
                                               config.window.windowMode);
    }
    else if (subCmd == "VSYNC")
    {
        int enable;
        ss >> enable;
        config.window.vsync = (enable != 0);
    }
    else if (subCmd == "MONITOR")
    {
        ss >> config.window.monitorIndex;
    }
    else if (subCmd == "REFRESH_RATE")
    {
        ss >> config.window.refreshRate;
    }
    else if (subCmd == "FPS")
    {
        ss >> config.window.frameRateLimit;
    }
    else if (subCmd == "FRUSTUM")
    {
        int enable;
        ss >> enable;
        config.culling.frustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "SPATIAL_CULLING")
    {
        std::string mode;
        ss >> mode;
        config.culling.spatialCullingMode =
            ResolveEnum("SPATIAL_CULLING", mode,
                        {{"AUTO", SpatialCullingMode::Auto},
                         {"LINEAR", SpatialCullingMode::Linear},
                         {"OCTREE", SpatialCullingMode::Octree}},
                        config.culling.spatialCullingMode);
    }
    else if (subCmd == "SHADOW_FRUSTUM")
    {
        int enable;
        ss >> enable;
        config.shadow.shadowFrustumCullingEnabled = (enable != 0);
    }
    else if (subCmd == "SHADOW_DISTANCE")
    {
        ss >> config.shadow.shadowDistanceCulling;
    }
    else if (subCmd == "SHADOW_BIAS")
    {
        ss >> config.shadow.shadowBias;
    }
    else if (subCmd == "SHADOW_SOFTNESS")
    {
        ss >> config.shadow.shadowSoftness;
    }
    else if (subCmd == "DISTANCE")
    {
        ss >> config.culling.distanceCulling;
    }
    else if (subCmd == "MOUSE_SENSITIVITY")
    {
        ss >> config.input.mouseSensitivityX;
        config.input.mouseSensitivityY = config.input.mouseSensitivityX;
    }
    else if (subCmd == "MOUSE_SENSITIVITY_X")
    {
        ss >> config.input.mouseSensitivityX;
    }
    else if (subCmd == "MOUSE_SENSITIVITY_Y")
    {
        ss >> config.input.mouseSensitivityY;
    }
    else if (subCmd == "MOUSE_INVERT_X")
    {
        int invert;
        ss >> invert;
        config.input.mouseInvertX = (invert != 0);
    }
    else if (subCmd == "MOUSE_INVERT_Y")
    {
        int invert;
        ss >> invert;
        config.input.mouseInvertY = (invert != 0);
    }
    else if (subCmd == "RAW_MOUSE_INPUT")
    {
        int enable;
        ss >> enable;
        config.input.rawMouseInput = (enable != 0);
    }
    else if (subCmd == "MSAA")
    {
        ss >> config.graphics.msaaSamples;
    }
    else if (subCmd == "ANISOTROPY")
    {
        ss >> config.graphics.maxAnisotropy;
    }
    else if (subCmd == "RENDER_SCALE")
    {
        ss >> config.graphics.renderScale;
    }
    else if (subCmd == "ASYNC_RESOURCES")
    {
        int enable;
        ss >> enable;
        config.graphics.asyncResourceLoading = (enable != 0);
    }
    else if (subCmd == "STRICT_ASSET_LOADING")
    {
        int enable;
        ss >> enable;
        config.graphics.strictAssetLoading = (enable != 0);
    }
    else if (subCmd == "SHADOWS_ENABLED")
    {
        int enable;
        ss >> enable;
        config.shadow.shadowsEnabled = (enable != 0);
    }
    else if (subCmd == "BLOOM_ENABLED")
    {
        int enable;
        ss >> enable;
        config.render.bloomEnabled = (enable != 0);
    }
    else if (subCmd == "HDR_ENABLED")
    {
        int enable;
        ss >> enable;
        config.render.hdrEnabled = (enable != 0);
    }
    else if (subCmd == "GAMMA")
    {
        ss >> config.render.gamma;
    }
    else if (subCmd == "EXPOSURE")
    {
        ss >> config.render.exposure;
    }
    else if (subCmd == "BLOOM_INTENSITY")
    {
        ss >> config.render.bloomIntensity;
    }
    else if (subCmd == "BLOOM_THRESHOLD")
    {
        ss >> config.render.bloomThreshold;
    }
    else if (subCmd == "BLOOM_RADIUS")
    {
        ss >> config.render.bloomRadius;
    }
    else if (subCmd == "SKYBOX_INTENSITY")
    {
        ss >> config.render.skyboxIntensity;
    }
    else if (subCmd == "AMBIENT_INTENSITY")
    {
        ss >> config.render.ambientIntensity;
    }
    else if (subCmd == "UI_REFERENCE_WIDTH")
    {
        ss >> config.render.uiReferenceWidth;
    }
    else if (subCmd == "UI_REFERENCE_HEIGHT")
    {
        ss >> config.render.uiReferenceHeight;
    }
    else if (subCmd == "UI_REFERENCE_SIZE")
    {
        ss >> config.render.uiReferenceWidth >> config.render.uiReferenceHeight;
    }
    else if (subCmd == "VOLUME")
    {
        ss >> config.audio.masterVolume;
    }
    else if (subCmd == "JOB_THREADS")
    {
        ss >> config.numJobThreads;
    }
    else if (subCmd == "TIME_SCALE")
    {
        ss >> config.timeScale;
    }
    else if (subCmd == "GRAVITY")
    {
        ss >> config.physics.gravity[0] >> config.physics.gravity[1] >> config.physics.gravity[2];
    }
    else if (subCmd == "MAX_SUBSTEPS")
    {
        ss >> config.physics.maxSubSteps;
    }
    else if (subCmd == "PHYSICS_TICKRATE")
    {
        ss >> config.physics.physicsTickRate;
    }
    else if (subCmd == "CCD_ENABLED")
    {
        int enable;
        ss >> enable;
        config.physics.ccdEnabled = (enable != 0);
    }
    else if (subCmd == "CCD_THRESHOLD")
    {
        ss >> config.physics.ccdThreshold;
    }
    else if (subCmd == "SOLVER_ITERATIONS")
    {
        ss >> config.physics.solverIterations;
    }
    else if (subCmd == "LIGHTING_MODE")
    {
        std::string val;
        ss >> val;
        config.lightingMode = ResolveEnum("LIGHTING_MODE", val,
                                          {{"BAKE", LightingMode::Bake},
                                           {"LIGHT_PROBE", LightingMode::LightProbe},
                                           {"LIGHTPROBE", LightingMode::LightProbe},
                                           {"REFLECTION_PROBES", LightingMode::ReflectionProbes},
                                           {"REFLECTIONPROBES", LightingMode::ReflectionProbes},
                                           {"REAL_TIME", LightingMode::RealTime},
                                           {"REALTIME", LightingMode::RealTime}},
                                          config.lightingMode);
    }
    else if (subCmd == "ANTIALIASING")
    {
        std::string val;
        ss >> val;
        config.graphics.antialiasing = ResolveEnum(
            "ANTIALIASING", val, {{"NONE", 0}, {"OFF", 0}, {"FXAA", 1}, {"TAA", 2}}, config.graphics.antialiasing);
    }
    else if (subCmd == "HEADLESS")
    {
        int enable;
        ss >> enable;
        config.headlessMode = (enable != 0);
    }
    else if (subCmd == "LOAD_DEFAULT_ASSETS")
    {
        int enable;
        ss >> enable;
        config.loadDefaultAssets = (enable != 0);
    }
    else if (subCmd == "DEFAULT_ASSET_MANIFEST")
    {
        config.defaultAssetManifest = ReadRemaining(ss);
    }
    else if (subCmd == "TITLE")
    {
        config.title = ReadRemaining(ss);
    }
    else if (subCmd == "ICON_PATH")
    {
        config.iconPath = ReadRemaining(ss);
    }
    else if (subCmd == "CLEAR_COLOR")
    {
        ss >> config.render.clearColor[0] >> config.render.clearColor[1] >> config.render.clearColor[2] >>
            config.render.clearColor[3];
    }
    else if (subCmd == "AUDIO_DEVICE")
    {
        config.audio.audioDevice = ReadRemaining(ss);
    }
    else if (subCmd == "AUDIO_CAPTURE_ENABLED")
    {
        int enable = 0;
        ss >> enable;
        config.audio.captureEnabled = enable != 0;
    }
    else if (subCmd == "AUDIO_CAPTURE_DEVICE")
    {
        config.audio.captureDevice = ReadRemaining(ss);
    }
    else if (subCmd == "AUDIO_CAPTURE_INPUT_VOLUME" || subCmd == "MIC_INPUT_VOLUME")
    {
        ss >> config.audio.captureInputVolume;
    }
    else if (subCmd == "AUDIO_CAPTURE_NOISE_GATE" || subCmd == "MIC_INPUT_THRESHOLD")
    {
        ss >> config.audio.captureNoiseGate;
    }
    else if (subCmd == "AUDIO_CAPTURE_GAIN")
    {
        ss >> config.audio.captureGain;
    }
    else if (subCmd == "AUDIO_CAPTURE_ATTACK_SECONDS")
    {
        ss >> config.audio.captureAttackSeconds;
    }
    else if (subCmd == "AUDIO_CAPTURE_RELEASE_SECONDS")
    {
        ss >> config.audio.captureReleaseSeconds;
    }
    else if (subCmd == "AUDIO_CAPTURE_PEAK_DECAY_SECONDS")
    {
        ss >> config.audio.capturePeakDecaySeconds;
    }
    else if (subCmd == "AUDIO_CAPTURE_CALIBRATION_SECONDS")
    {
        ss >> config.audio.captureCalibrationSeconds;
    }
    else if (subCmd == "AUDIO_CAPTURE_PULSE_THRESHOLD")
    {
        ss >> config.audio.capturePulseThreshold;
    }
    else if (subCmd == "AUDIO_CAPTURE_PULSE_COOLDOWN")
    {
        ss >> config.audio.capturePulseCooldown;
    }
    else if (subCmd == "AUDIO_CAPTURE_PULSE_DURATION")
    {
        ss >> config.audio.capturePulseDuration;
    }
    else if (subCmd == "OCCLUSION_CULLING")
    {
        int enable;
        ss >> enable;
        config.culling.occlusionCullingEnabled = (enable != 0);
    }
    else if (subCmd == "RENDER_ORDER")
    {
        int enable;
        ss >> enable;
        config.culling.renderOrderEnabled = (enable != 0);
    }
    else if (subCmd == "FILTER_LAYER")
    {
        uint32_t mask;
        ss >> mask;
        config.culling.filterLayerMask = mask;
    }
    else if (LoadOptimizationSetting(subCmd, ss, config.optimization))
    {
    }

    else if (subCmd == "PHYSICS_DEBUG")
    {
        int enable;
        ss >> enable;
        config.debug.physicsDebug = (enable != 0);
    }
    else if (subCmd == "UI_ENABLED")
    {
        int enable;
        ss >> enable;
        config.debug.uiEnabled = (enable != 0);
    }
    else if (subCmd == "GIZMOS")
    {
        int enable;
        ss >> enable;
        config.debug.gizmos = (enable != 0);
    }
    else if (subCmd == "LIGHT_GIZMOS")
    {
        int enable;
        ss >> enable;
        config.debug.lightGizmos = (enable != 0);
    }
    else if (subCmd == "ENTITY_NAMES")
    {
        int enable;
        ss >> enable;
        config.debug.entityNames = (enable != 0);
    }
    else if (subCmd == "AUDIO_DEBUG")
    {
        int enable;
        ss >> enable;
        config.debug.audioDebug = (enable != 0);
    }
    else if (subCmd == "PARTICLE_DEBUG")
    {
        int enable;
        ss >> enable;
        config.debug.particleDebug = (enable != 0);
    }
    else if (subCmd == "GRID_SNAP_ENABLED")
    {
        int enable;
        ss >> enable;
        config.debug.gridSnapEnabled = (enable != 0);
    }
    else if (subCmd == "GRID_INDICATOR_ENABLED")
    {
        int enable;
        ss >> enable;
        config.debug.gridIndicatorEnabled = (enable != 0);
    }
    else if (subCmd == "GRID_SNAP_TRANSLATION")
    {
        ss >> config.debug.gridSnapTranslation;
    }
    else if (subCmd == "GRID_SNAP_ROTATION")
    {
        ss >> config.debug.gridSnapRotation;
    }
    else if (subCmd == "GRID_SNAP_SCALE")
    {
        ss >> config.debug.gridSnapScale;
    }
    else
    {
        if (!subCmd.empty())
        {
            LOGGER_WARN("ConfigLoader") << "Unknown config key: " << subCmd;
        }
    }
}
