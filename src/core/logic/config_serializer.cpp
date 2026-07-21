#include <core/logic/config_serializer.h>
#include <core/logic/config_loader.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/yaml_writer.h>
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

bool ConfigSerializer::Deserialize(const std::string& filepath, AppConfig& config)
{
    bool headless = m_Headless;
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
            LOGGER_WARN("ConfigSerializer")
                << "Potential typo in root key: '" << root.key << "', expected 'axis_config' in " << filepath;
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

    // Accumulate all config entries as flat YAMLNodes, then emit in one pass
    std::vector<YAMLNode> entries;
    entries.reserve(112);

    auto kv = [&](const std::string& key, const std::string& value) { entries.push_back({key, value, {}}); };

    // --- Core Settings ---
    kv("TITLE", cfg.title);

    auto LogLevelToStr = [](LogLevel level) {
        switch (level)
        {
            case LogLevel::None:
                return "NONE";
            case LogLevel::Minimal:
                return "MINIMAL";
            case LogLevel::Flex:
                return "FLEX";
            case LogLevel::Verbose:
                return "VERBOSE";
            case LogLevel::Debug:
                return "DEBUG";
            default:
                return "DEBUG";
        }
    };
    kv("LOG_LEVEL", LogLevelToStr(cfg.logLevel));
    kv("JOB_THREADS", std::to_string(cfg.numJobThreads));
    kv("TIME_SCALE", FloatStr(cfg.timeScale));
    kv("ICON_PATH", cfg.iconPath);
    kv("HEADLESS", cfg.headlessMode ? "1" : "0");
    kv("LOAD_DEFAULT_ASSETS", cfg.loadDefaultAssets ? "1" : "0");
    kv("DEFAULT_ASSET_MANIFEST", cfg.defaultAssetManifest);

    // --- Window Settings ---
    kv("WINDOW_WIDTH", std::to_string(cfg.window.width));
    kv("WINDOW_HEIGHT", std::to_string(cfg.window.height));

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
    kv("WINDOW_MODE", WindowModeToStr(cfg.window.windowMode));
    kv("VSYNC", cfg.window.vsync ? "1" : "0");
    kv("MONITOR", std::to_string(cfg.window.monitorIndex));
    kv("REFRESH_RATE", std::to_string(cfg.window.refreshRate));
    kv("FPS", std::to_string(cfg.window.frameRateLimit));

    // --- Graphics Settings ---
    auto GraphicsBackendToStr = [](GraphicsBackend backend) {
        switch (backend)
        {
            case GraphicsBackend::Vulkan:
                return "VULKAN";
            case GraphicsBackend::DirectX:
                return "DIRECTX";
            default:
                return "OPENGL";
        }
    };
    kv("GRAPHICS_API", GraphicsBackendToStr(cfg.graphics.graphicsBackend));
    kv("MSAA", std::to_string(cfg.graphics.msaaSamples));

    auto AntialiasingToStr = [](int aa) {
        switch (aa)
        {
            case 0:
                return "NONE";
            case 1:
                return "FXAA";
            case 2:
                return "TAA";
            default:
                return "NONE";
        }
    };
    kv("ANTIALIASING", AntialiasingToStr(cfg.graphics.antialiasing));
    kv("ANISOTROPY", FloatStr(cfg.graphics.maxAnisotropy));
    kv("RENDER_SCALE", FloatStr(cfg.graphics.renderScale));
    kv("ASYNC_RESOURCES", cfg.graphics.asyncResourceLoading ? "1" : "0");
    kv("STRICT_ASSET_LOADING", cfg.graphics.strictAssetLoading ? "1" : "0");

    // --- Render Settings ---
    kv("HDR_ENABLED", cfg.render.hdrEnabled ? "1" : "0");
    kv("BLOOM_ENABLED", cfg.render.bloomEnabled ? "1" : "0");
    kv("GAMMA", FloatStr(cfg.render.gamma));
    kv("EXPOSURE", FloatStr(cfg.render.exposure));
    kv("BLOOM_INTENSITY", FloatStr(cfg.render.bloomIntensity));
    kv("BLOOM_THRESHOLD", FloatStr(cfg.render.bloomThreshold));
    kv("BLOOM_RADIUS", FloatStr(cfg.render.bloomRadius));
    kv("SKYBOX_INTENSITY", FloatStr(cfg.render.skyboxIntensity));
    kv("AMBIENT_INTENSITY", FloatStr(cfg.render.ambientIntensity));
    kv("UI_REFERENCE_SIZE", FloatStr(cfg.render.uiReferenceWidth) + " " + FloatStr(cfg.render.uiReferenceHeight));

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
    kv("TONEMAPPING", TonemappingToStr(cfg.render.tonemappingMode));

    std::string clrColor = FloatStr(cfg.render.clearColor[0]) + " " + FloatStr(cfg.render.clearColor[1]) + " " +
                           FloatStr(cfg.render.clearColor[2]) + " " + FloatStr(cfg.render.clearColor[3]);
    kv("CLEAR_COLOR", clrColor);

    // --- Shadow Settings ---
    kv("SHADOWS_ENABLED", cfg.shadow.shadowsEnabled ? "1" : "0");
    kv("SHADOWS", std::to_string(cfg.shadow.shadowMode));
    kv("SHADOW_RESOLUTION", std::to_string(cfg.shadow.shadowMapResolution));
    kv("SHADOW_SIZE", FloatStr(cfg.shadow.shadowProjectionSize));
    kv("SHADOW_FRUSTUM", cfg.shadow.shadowFrustumCullingEnabled ? "1" : "0");
    kv("SHADOW_DISTANCE", FloatStr(cfg.shadow.shadowDistanceCulling));
    kv("SHADOW_BIAS", FloatStr(cfg.shadow.shadowBias));
    kv("SHADOW_SOFTNESS", std::to_string(cfg.shadow.shadowSoftness));

    // --- Physics Settings ---
    auto PhysicsBackendToStr = [](PhysicsBackend backend) {
        switch (backend)
        {
            case PhysicsBackend::PhysX:
                return "PHYSX";
            default:
                return "BULLET";
        }
    };
    kv("PHYSICS_ENGINE", PhysicsBackendToStr(cfg.physics.physicsBackend));

    auto PhysicsModeToStr = [](PhysicsMode mode) {
        switch (mode)
        {
            case PhysicsMode::Fast:
                return "FAST";
            case PhysicsMode::Accurate:
                return "ACCURATE";
            default:
                return "BALANCED";
        }
    };
    kv("PHYSICS_MODE", PhysicsModeToStr(cfg.physics.physicsMode));
    kv("GRAVITY", FormatGravity(cfg.physics.gravity));
    kv("MAX_SUBSTEPS", std::to_string(cfg.physics.maxSubSteps));
    kv("PHYSICS_TICKRATE", FloatStr(cfg.physics.physicsTickRate));
    kv("CCD_ENABLED", cfg.physics.ccdEnabled ? "1" : "0");
    kv("CCD_THRESHOLD", FloatStr(cfg.physics.ccdThreshold));
    kv("SOLVER_ITERATIONS", std::to_string(cfg.physics.solverIterations));

    // --- Input Settings ---
    kv("MOUSE_SENSITIVITY_X", FloatStr(cfg.input.mouseSensitivityX));
    kv("MOUSE_SENSITIVITY_Y", FloatStr(cfg.input.mouseSensitivityY));
    kv("MOUSE_INVERT_X", cfg.input.mouseInvertX ? "1" : "0");
    kv("MOUSE_INVERT_Y", cfg.input.mouseInvertY ? "1" : "0");
    kv("RAW_MOUSE_INPUT", cfg.input.rawMouseInput ? "1" : "0");

    // --- Audio Settings ---
    auto AudioBackendToStr = [](AudioBackend backend) {
        switch (backend)
        {
            case AudioBackend::IrrKlang:
                return "IRRKLANG";
            case AudioBackend::FMOD:
                return "FMOD";
            case AudioBackend::OpenAL:
                return "OPENAL";
            default:
                return "NULL";
        }
    };
    kv("AUDIO_ENGINE", AudioBackendToStr(cfg.audio.audioBackend));
    kv("VOLUME", FloatStr(cfg.audio.masterVolume));
    kv("AUDIO_DEVICE", cfg.audio.audioDevice);
    kv("AUDIO_CAPTURE_ENABLED", cfg.audio.captureEnabled ? "1" : "0");
    kv("AUDIO_CAPTURE_DEVICE", cfg.audio.captureDevice);
    kv("AUDIO_CAPTURE_INPUT_VOLUME", FloatStr(cfg.audio.captureInputVolume));
    kv("AUDIO_CAPTURE_NOISE_GATE", FloatStr(cfg.audio.captureNoiseGate));
    kv("AUDIO_CAPTURE_GAIN", FloatStr(cfg.audio.captureGain));
    kv("AUDIO_CAPTURE_ATTACK_SECONDS", FloatStr(cfg.audio.captureAttackSeconds));
    kv("AUDIO_CAPTURE_RELEASE_SECONDS", FloatStr(cfg.audio.captureReleaseSeconds));
    kv("AUDIO_CAPTURE_PEAK_DECAY_SECONDS", FloatStr(cfg.audio.capturePeakDecaySeconds));
    kv("AUDIO_CAPTURE_CALIBRATION_SECONDS", FloatStr(cfg.audio.captureCalibrationSeconds));
    kv("AUDIO_CAPTURE_PULSE_THRESHOLD", FloatStr(cfg.audio.capturePulseThreshold));
    kv("AUDIO_CAPTURE_PULSE_COOLDOWN", FloatStr(cfg.audio.capturePulseCooldown));
    kv("AUDIO_CAPTURE_PULSE_DURATION", FloatStr(cfg.audio.capturePulseDuration));

    // --- Culling Settings ---
    kv("CULL_FACE", cfg.culling.cullFaceEnabled ? "1" : "0");
    kv("DEPTH_TEST", cfg.culling.depthTestEnabled ? "1" : "0");
    kv("STENCIL_TEST", cfg.culling.stencilTestEnabled ? "1" : "0");
    kv("FRUSTUM", cfg.culling.frustumCullingEnabled ? "1" : "0");
    kv("SPATIAL_CULLING", SpatialCullingModeName(cfg.culling.spatialCullingMode));
    kv("OCCLUSION_CULLING", cfg.culling.occlusionCullingEnabled ? "1" : "0");
    kv("INSTANCING", cfg.culling.instanceBatchingEnabled ? "1" : "0");
    kv("RENDER_ORDER", cfg.culling.renderOrderEnabled ? "1" : "0");
    kv("FILTER_LAYER", std::to_string(cfg.culling.filterLayerMask));
    kv("DISTANCE", FloatStr(cfg.culling.distanceCulling));

    // --- Runtime Optimization Settings ---
    kv("OPT_RESOURCE_HOT_RELOAD", cfg.optimization.resourceHotReloadEnabled ? "1" : "0");
    kv("OPT_RESOURCE_UPLOAD_BUDGET", cfg.optimization.resourceUploadBudgetEnabled ? "1" : "0");
    kv("OPT_MAX_MODEL_UPLOADS_PER_FRAME", std::to_string(cfg.optimization.maxModelUploadsPerFrame));
    kv("OPT_MAX_TEXTURE_UPLOADS_PER_FRAME", std::to_string(cfg.optimization.maxTextureUploadsPerFrame));
    kv("OPT_DISCARD_CPU_MESH_DATA_AFTER_UPLOAD",
       cfg.optimization.discardCpuMeshDataAfterUpload ? "1" : "0");
    kv("OPT_COMPRESSED_TEXTURE_LOADING", cfg.optimization.compressedTextureLoadingEnabled ? "1" : "0");
    kv("OPT_STREAMING_UPDATE_THROTTLING",
       cfg.optimization.streamingUpdateThrottlingEnabled ? "1" : "0");
    kv("OPT_STREAMING_CHECK_INTERVAL", FloatStr(cfg.optimization.streamingCheckIntervalSeconds));
    kv("OPT_REFLECTION_CAPTURE_BUDGET", cfg.optimization.reflectionCaptureBudgetEnabled ? "1" : "0");
    kv("OPT_MAX_REFLECTION_PROBE_FACES_PER_FRAME",
       std::to_string(cfg.optimization.maxReflectionProbeFacesPerFrame));
    kv("OPT_MAX_PLANAR_REFLECTION_CAPTURES_PER_FRAME",
       std::to_string(cfg.optimization.maxPlanarReflectionCapturesPerFrame));
    kv("OPT_SHADOW_PARALLEL_BUILD", cfg.optimization.shadowParallelBuildEnabled ? "1" : "0");
    kv("OPT_SHADOW_PARALLEL_THRESHOLD", std::to_string(cfg.optimization.shadowParallelThreshold));
    kv("OPT_ANIMATION_PARALLEL_EVALUATION",
       cfg.optimization.animationParallelEvaluationEnabled ? "1" : "0");
    kv("OPT_ANIMATION_PARALLEL_THRESHOLD", std::to_string(cfg.optimization.animationParallelThreshold));
    kv("OPT_NAVIGATION_SPATIAL_HASH", cfg.optimization.navigationSpatialHashEnabled ? "1" : "0");
    kv("OPT_NAVIGATION_AGENT_CELL_SIZE", FloatStr(cfg.optimization.navigationAgentCellSize));
    kv("OPT_NAVIGATION_ASYNC_PATHFINDING", cfg.optimization.navigationAsyncPathfindingEnabled ? "1" : "0");
    kv("OPT_NAVIGATION_MAX_PATH_REQUESTS_PER_FRAME",
       std::to_string(cfg.optimization.navigationMaxPathRequestsPerFrame));
    kv("OPT_NAVMESH_REBUILD_BUDGET", cfg.optimization.navMeshRebuildBudgetEnabled ? "1" : "0");
    kv("OPT_MAX_NAVMESH_REBUILDS_PER_FRAME", std::to_string(cfg.optimization.maxNavMeshRebuildsPerFrame));
    kv("OPT_NAVIGATION_DIRTY_TILES", cfg.optimization.navigationDirtyTilesEnabled ? "1" : "0");
    kv("OPT_NAVIGATION_NAVMESH_TILE_SIZE", FloatStr(cfg.optimization.navigationNavMeshTileSize));
    kv("OPT_NAVIGATION_MAX_DIRTY_TILES_PER_FRAME",
       std::to_string(cfg.optimization.navigationMaxDirtyTilesPerFrame));
    kv("OPT_NETWORK_BATCHING", cfg.optimization.networkBatchingEnabled ? "1" : "0");
    kv("OPT_NETWORK_MAX_EVENTS_PER_UPDATE", std::to_string(cfg.optimization.networkMaxEventsPerUpdate));
    kv("OPT_NETWORK_MAX_EVENT_PROCESSING_MS", FloatStr(cfg.optimization.networkMaxEventProcessingMs));
    kv("OPT_NETWORK_MAX_BYTES_PER_UPDATE", std::to_string(cfg.optimization.networkMaxBytesPerUpdate));
    kv("OPT_NETWORK_REPLICATION", cfg.optimization.networkReplicationEnabled ? "1" : "0");
    kv("OPT_NETWORK_REPLICATION_RATE_HZ", FloatStr(cfg.optimization.networkReplicationRateHz));
    kv("OPT_NETWORK_INTEREST_RADIUS", FloatStr(cfg.optimization.networkInterestRadius));
    kv("OPT_PARTICLE_SPAWN_BUDGET", cfg.optimization.particleSpawnBudgetEnabled ? "1" : "0");
    kv("OPT_PARTICLE_MAX_SPAWN_PER_FRAME", std::to_string(cfg.optimization.particleMaxSpawnPerFrame));
    kv("OPT_PARTICLE_BATCHING", cfg.optimization.particleBatchingEnabled ? "1" : "0");
    kv("OPT_RENDER_STATE_CACHE", cfg.optimization.renderStateCacheEnabled ? "1" : "0");
    kv("OPT_PERSISTENT_MAPPED_BUFFERS", cfg.optimization.persistentMappedBuffersEnabled ? "1" : "0");
    kv("OPT_TILED_LIGHT_CULLING", cfg.optimization.tiledLightCullingEnabled ? "1" : "0");
    kv("OPT_TILED_LIGHT_TILE_SIZE", std::to_string(cfg.optimization.tiledLightTileSize));
    kv("OPT_GBUFFER_ENTITY_ID", cfg.optimization.gbufferEntityIdEnabled ? "1" : "0");
    kv("OPT_PHYSICS_MESH_SHAPE_CACHE", cfg.optimization.physicsMeshShapeCacheEnabled ? "1" : "0");
    kv("OPT_UI_LAYOUT_CACHE", cfg.optimization.uiLayoutCacheEnabled ? "1" : "0");
    kv("OPT_VIDEO_ASYNC_DECODE", cfg.optimization.videoAsyncDecodeEnabled ? "1" : "0");

    // --- Debug Settings ---

    kv("PHYSICS_DEBUG", cfg.debug.physicsDebug ? "1" : "0");
    kv("UI_ENABLED", cfg.debug.uiEnabled ? "1" : "0");
    kv("GIZMOS", cfg.debug.gizmos ? "1" : "0");
    kv("LIGHT_GIZMOS", cfg.debug.lightGizmos ? "1" : "0");
    kv("ENTITY_NAMES", cfg.debug.entityNames ? "1" : "0");
    kv("AUDIO_DEBUG", cfg.debug.audioDebug ? "1" : "0");
    kv("PARTICLE_DEBUG", cfg.debug.particleDebug ? "1" : "0");
    kv("GRID_SNAP_ENABLED", cfg.debug.gridSnapEnabled ? "1" : "0");
    kv("GRID_INDICATOR_ENABLED", cfg.debug.gridIndicatorEnabled ? "1" : "0");
    kv("GRID_SNAP_TRANSLATION", FloatStr(cfg.debug.gridSnapTranslation));
    kv("GRID_SNAP_ROTATION", FloatStr(cfg.debug.gridSnapRotation));
    kv("GRID_SNAP_SCALE", FloatStr(cfg.debug.gridSnapScale));

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
    kv("LIGHTING_MODE", LightingModeToStr(cfg.lightingMode));

    // Emit: axis_config:\n then all entries at indent 4 (matches original format)
    f << "axis_config:\n";
    YAMLWriter::Write(f, entries, 4);

    return true;
}
