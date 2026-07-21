#include <core/logic/config_validation.h>
#include <audio/interface/i_audio_capture_service.h>
#include <audio/logic/audio_capture_processor.h>
#include <core/logic/backend_registry.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace
{
template <typename T>
void ClampValue(T& value, T minimum, T maximum, const char* field, std::vector<ConfigValidationIssue>& issues)
{
    const T sanitized = std::clamp(value, minimum, maximum);
    if (sanitized == value)
        return;
    value = sanitized;
    issues.push_back({field, "value was outside the supported range"});
}

void FiniteOr(float& value, float fallback, const char* field, std::vector<ConfigValidationIssue>& issues)
{
    if (std::isfinite(value))
        return;
    value = fallback;
    issues.push_back({field, "non-finite value was replaced by the default"});
}

void ClampFinite(float& value, float minimum, float maximum, float fallback, const char* field,
                 std::vector<ConfigValidationIssue>& issues)
{
    FiniteOr(value, fallback, field, issues);
    ClampValue(value, minimum, maximum, field, issues);
}
}  // namespace

ConfigValidationResult ValidateAndSanitizeConfig(const AppConfig& input, const ConfigValidationPolicy& policy)
{
    ConfigValidationResult result;
    result.config = input;
    auto& config = result.config;
    if (config.loadDefaultAssets && config.defaultAssetManifest.empty())
    {
        config.defaultAssetManifest = "asset://load.axs";
        result.issues.push_back({"defaultAssetManifest", "empty manifest replaced with engine default"});
    }
    auto& issues = result.issues;

    if (!policy.allowUncompiledGraphicsBackend && !BackendRegistry::IsSupported(config.graphics.graphicsBackend))
    {
#if AXIS_HAS_OPENGL_BACKEND
        config.graphics.graphicsBackend = GraphicsBackend::OpenGL;
#endif
        issues.push_back({"graphics.graphicsBackend", "backend is not compiled; selected the build default"});
    }
    if (!policy.allowUncompiledPhysicsBackend && !BackendRegistry::IsSupported(config.physics.physicsBackend))
    {
#if AXIS_HAS_BULLET_BACKEND
        config.physics.physicsBackend = PhysicsBackend::Bullet;
#endif
        issues.push_back({"physics.physicsBackend", "backend is not compiled; selected the build default"});
    }
    if (!policy.allowUncompiledAudioBackend && !BackendRegistry::IsSupported(config.audio.audioBackend))
    {
#if AXIS_HAS_IRRKLANG_BACKEND
        config.audio.audioBackend = AudioBackend::IrrKlang;
#elif AXIS_HAS_FMOD_BACKEND
        config.audio.audioBackend = AudioBackend::FMOD;
#else
        config.audio.audioBackend = AudioBackend::Null;
#endif
        issues.push_back({"audio.audioBackend", "backend is not compiled; selected the build default"});
    }

    ClampValue(config.window.width, 1, 16384, "window.width", issues);
    ClampValue(config.window.height, 1, 16384, "window.height", issues);
    ClampValue(config.window.monitorIndex, 0, 255, "window.monitorIndex", issues);
    ClampValue(config.window.refreshRate, 0, 1000, "window.refreshRate", issues);
    ClampValue(config.window.frameRateLimit, 0, 10000, "window.frameRateLimit", issues);
    ClampValue(config.numJobThreads, -1, 1024, "numJobThreads", issues);

    ClampValue(config.graphics.msaaSamples, 0, 64, "graphics.msaaSamples", issues);
    ClampValue(config.graphics.antialiasing, 0, 3, "graphics.antialiasing", issues);
    ClampFinite(config.graphics.maxAnisotropy, 1.0f, 64.0f, 1.0f, "graphics.maxAnisotropy", issues);
    ClampFinite(config.graphics.renderScale, 0.1f, 4.0f, 1.0f, "graphics.renderScale", issues);

    ClampFinite(config.render.gamma, 0.1f, 8.0f, 2.2f, "render.gamma", issues);
    ClampFinite(config.render.exposure, 0.0f, 100.0f, 1.0f, "render.exposure", issues);
    ClampFinite(config.render.bloomIntensity, 0.0f, 100.0f, 1.0f, "render.bloomIntensity", issues);
    ClampFinite(config.render.bloomThreshold, 0.0f, 100.0f, 1.0f, "render.bloomThreshold", issues);
    ClampFinite(config.render.bloomRadius, 0.0f, 1.0f, 0.005f, "render.bloomRadius", issues);
    ClampFinite(config.render.skyboxIntensity, 0.0f, 100.0f, 1.0f, "render.skyboxIntensity", issues);
    ClampFinite(config.render.ambientIntensity, 0.0f, 100.0f, 1.0f, "render.ambientIntensity", issues);
    ClampFinite(config.render.uiReferenceWidth, 1.0f, 16384.0f, 1920.0f, "render.uiReferenceWidth", issues);
    ClampFinite(config.render.uiReferenceHeight, 1.0f, 16384.0f, 1080.0f, "render.uiReferenceHeight", issues);
    for (int channel = 0; channel < 4; ++channel)
        ClampFinite(config.render.clearColor[channel], 0.0f, 1.0f, channel == 3 ? 1.0f : 0.1f,
                    ("render.clearColor[" + std::to_string(channel) + "]").c_str(), issues);

    ClampValue(config.shadow.shadowMapResolution, 1, 16384, "shadow.shadowMapResolution", issues);
    ClampValue(config.shadow.shadowMode, 0, 2, "shadow.shadowMode", issues);
    ClampFinite(config.shadow.shadowProjectionSize, 0.01f, 100000.0f, 100.0f, "shadow.shadowProjectionSize", issues);
    ClampFinite(config.shadow.shadowDistanceCulling, 0.0f, 1000000.0f, 100.0f, "shadow.shadowDistanceCulling", issues);
    ClampFinite(config.shadow.shadowBias, 0.0f, 1.0f, 0.005f, "shadow.shadowBias", issues);
    ClampValue(config.shadow.shadowSoftness, 0, 64, "shadow.shadowSoftness", issues);

    ClampValue(config.physics.maxSubSteps, 1, 128, "physics.maxSubSteps", issues);
    ClampFinite(config.physics.physicsTickRate, 1.0f, 10000.0f, 60.0f, "physics.physicsTickRate", issues);
    ClampFinite(config.physics.ccdThreshold, 0.0f, 1000000.0f, 0.0f, "physics.ccdThreshold", issues);
    ClampValue(config.physics.solverIterations, 1, 1024, "physics.solverIterations", issues);
    for (int axis = 0; axis < 3; ++axis)
        FiniteOr(config.physics.gravity[axis], axis == 1 ? -9.81f : 0.0f,
                 ("physics.gravity[" + std::to_string(axis) + "]").c_str(), issues);

    ClampFinite(config.input.mouseSensitivityX, 0.0f, 100.0f, 0.1f, "input.mouseSensitivityX", issues);
    ClampFinite(config.input.mouseSensitivityY, 0.0f, 100.0f, 0.1f, "input.mouseSensitivityY", issues);
    ClampFinite(config.timeScale, 0.0f, 100.0f, 1.0f, "timeScale", issues);
    ClampFinite(config.audio.masterVolume, 0.0f, 100.0f, 100.0f, "audio.masterVolume", issues);

    AudioCaptureSettings capture;
    capture.inputVolume = config.audio.captureInputVolume;
    capture.noiseGate = config.audio.captureNoiseGate;
    capture.gain = config.audio.captureGain;
    capture.attackSeconds = config.audio.captureAttackSeconds;
    capture.releaseSeconds = config.audio.captureReleaseSeconds;
    capture.peakDecaySeconds = config.audio.capturePeakDecaySeconds;
    capture.calibrationSeconds = config.audio.captureCalibrationSeconds;
    capture.pulseThreshold = config.audio.capturePulseThreshold;
    capture.pulseCooldown = config.audio.capturePulseCooldown;
    capture.pulseDuration = config.audio.capturePulseDuration;
    auto finiteCapture = [&](float& value, float fallback, const char* field) {
        if (!std::isfinite(value))
        {
            value = fallback;
            issues.push_back({field, "non-finite microphone processing value was replaced by the default"});
        }
    };
    finiteCapture(capture.inputVolume, 1.0f, "audio.captureInputVolume");
    finiteCapture(capture.noiseGate, 0.02f, "audio.captureNoiseGate");
    finiteCapture(capture.gain, 4.0f, "audio.captureGain");
    finiteCapture(capture.attackSeconds, 0.05f, "audio.captureAttackSeconds");
    finiteCapture(capture.releaseSeconds, 0.05f, "audio.captureReleaseSeconds");
    finiteCapture(capture.peakDecaySeconds, 0.125f, "audio.capturePeakDecaySeconds");
    finiteCapture(capture.calibrationSeconds, 1.0f, "audio.captureCalibrationSeconds");
    finiteCapture(capture.pulseThreshold, 0.15f, "audio.capturePulseThreshold");
    finiteCapture(capture.pulseCooldown, 0.08f, "audio.capturePulseCooldown");
    finiteCapture(capture.pulseDuration, 0.6f, "audio.capturePulseDuration");
    const AudioCaptureSettings sanitizedCapture = AudioCaptureProcessor::SanitizeSettings(capture);
    auto updateCapture = [&](float& destination, float sanitized, const char* field) {
        if (!std::isfinite(destination) || destination != sanitized)
        {
            destination = sanitized;
            issues.push_back({field, "microphone processing value was sanitized"});
        }
    };
    updateCapture(config.audio.captureInputVolume, sanitizedCapture.inputVolume, "audio.captureInputVolume");
    updateCapture(config.audio.captureNoiseGate, sanitizedCapture.noiseGate, "audio.captureNoiseGate");
    updateCapture(config.audio.captureGain, sanitizedCapture.gain, "audio.captureGain");
    updateCapture(config.audio.captureAttackSeconds, sanitizedCapture.attackSeconds, "audio.captureAttackSeconds");
    updateCapture(config.audio.captureReleaseSeconds, sanitizedCapture.releaseSeconds, "audio.captureReleaseSeconds");
    updateCapture(config.audio.capturePeakDecaySeconds, sanitizedCapture.peakDecaySeconds,
                  "audio.capturePeakDecaySeconds");
    updateCapture(config.audio.captureCalibrationSeconds, sanitizedCapture.calibrationSeconds,
                  "audio.captureCalibrationSeconds");
    updateCapture(config.audio.capturePulseThreshold, sanitizedCapture.pulseThreshold, "audio.capturePulseThreshold");
    updateCapture(config.audio.capturePulseCooldown, sanitizedCapture.pulseCooldown, "audio.capturePulseCooldown");
    updateCapture(config.audio.capturePulseDuration, sanitizedCapture.pulseDuration, "audio.capturePulseDuration");

    ClampFinite(config.culling.distanceCulling, 0.0f, 10000000.0f, 0.0f, "culling.distanceCulling", issues);
    ClampValue(config.optimization.maxModelUploadsPerFrame, 1, 65536,
               "optimization.maxModelUploadsPerFrame", issues);
    ClampValue(config.optimization.maxTextureUploadsPerFrame, 1, 65536,
               "optimization.maxTextureUploadsPerFrame", issues);
    ClampFinite(config.optimization.streamingCheckIntervalSeconds, 0.0f, 60.0f, 1.0f,
                "optimization.streamingCheckIntervalSeconds", issues);
    ClampValue(config.optimization.maxReflectionProbeFacesPerFrame, 1, 65536,
               "optimization.maxReflectionProbeFacesPerFrame", issues);
    ClampValue(config.optimization.maxPlanarReflectionCapturesPerFrame, 1, 65536,
               "optimization.maxPlanarReflectionCapturesPerFrame", issues);
    ClampValue(config.optimization.shadowParallelThreshold, 1, 10000000,
               "optimization.shadowParallelThreshold", issues);
    ClampValue(config.optimization.animationParallelThreshold, 1, 10000000,
               "optimization.animationParallelThreshold", issues);
    ClampFinite(config.optimization.navigationAgentCellSize, 0.01f, 100000.0f, 2.0f,
                "optimization.navigationAgentCellSize", issues);
    ClampValue(config.optimization.navigationMaxPathRequestsPerFrame, 1, 1000000,
               "optimization.navigationMaxPathRequestsPerFrame", issues);
    ClampValue(config.optimization.maxNavMeshRebuildsPerFrame, 1, 1000000,
               "optimization.maxNavMeshRebuildsPerFrame", issues);
    ClampFinite(config.optimization.navigationNavMeshTileSize, 0.25f, 100000.0f, 8.0f,
                "optimization.navigationNavMeshTileSize", issues);
    ClampValue(config.optimization.navigationMaxDirtyTilesPerFrame, 1, 1000000,
               "optimization.navigationMaxDirtyTilesPerFrame", issues);
    ClampValue(config.optimization.networkMaxEventsPerUpdate, 1, 1000000,
               "optimization.networkMaxEventsPerUpdate", issues);
    ClampFinite(config.optimization.networkMaxEventProcessingMs, 0.01f, 1000.0f, 2.0f,
                "optimization.networkMaxEventProcessingMs", issues);
    ClampValue(config.optimization.networkMaxBytesPerUpdate, 1, 1024 * 1024 * 1024,
               "optimization.networkMaxBytesPerUpdate", issues);
    ClampFinite(config.optimization.networkReplicationRateHz, 0.1f, 240.0f, 20.0f,
                "optimization.networkReplicationRateHz", issues);
    ClampFinite(config.optimization.networkInterestRadius, 0.0f, 10000000.0f, 0.0f,
                "optimization.networkInterestRadius", issues);
    ClampValue(config.optimization.particleMaxSpawnPerFrame, 1, 10000000,
               "optimization.particleMaxSpawnPerFrame", issues);
    ClampValue(config.optimization.tiledLightTileSize, 8, 64,
               "optimization.tiledLightTileSize", issues);
    ClampFinite(config.debug.gridSnapTranslation, 0.0001f, 100000.0f, 1.0f, "debug.gridSnapTranslation", issues);
    ClampFinite(config.debug.gridSnapRotation, 0.0001f, 360.0f, 15.0f, "debug.gridSnapRotation", issues);
    ClampFinite(config.debug.gridSnapScale, 0.0001f, 100000.0f, 0.25f, "debug.gridSnapScale", issues);

    return result;
}
