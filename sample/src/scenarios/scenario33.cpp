#include "sample_scenario_common.h"

#include <audio/interface/i_audio_capture_service.h>
#include <core/logic/config_manager.h>

void SampleState::ApplyScenario33CaptureSettings()
{
    auto* capture = Resolve<IAudioCaptureService>();
    if (!capture)
        return;

    AudioCaptureSettings settings = capture->GetSettings();
    settings.inputVolume = m_S33InputVolume;
    settings.noiseGate = m_S33NoiseGate;
    settings.gain = m_S33Gain;
    settings.attackSeconds = m_S33AttackSeconds;
    settings.releaseSeconds = m_S33ReleaseSeconds;
    settings.peakDecaySeconds = m_S33PeakDecaySeconds;
    settings.calibrationSeconds = m_S33CalibrationSeconds;
    settings.pulseThreshold = m_S33PulseThreshold;
    settings.pulseCooldown = m_S33PulseCooldown;
    settings.pulseDuration = m_S33PulseDuration;
    capture->SetSettings(settings);

    const auto sanitized = capture->GetSettings();
    m_S33InputVolume = sanitized.inputVolume;
    m_S33NoiseGate = sanitized.noiseGate;
    m_S33Gain = sanitized.gain;
    m_S33AttackSeconds = sanitized.attackSeconds;
    m_S33ReleaseSeconds = sanitized.releaseSeconds;
    m_S33PeakDecaySeconds = sanitized.peakDecaySeconds;
    m_S33CalibrationSeconds = sanitized.calibrationSeconds;
    m_S33PulseThreshold = sanitized.pulseThreshold;
    m_S33PulseCooldown = sanitized.pulseCooldown;
    m_S33PulseDuration = sanitized.pulseDuration;
}

void SampleState::RefreshScenario33Capture(bool startIfAvailable)
{
    auto* capture = Resolve<IAudioCaptureService>();
    m_S33DeviceDetected = false;
    if (!capture)
    {
        m_S33LastResult = AudioCaptureResult::Unsupported;
        return;
    }

    capture->RefreshDevices();
    const auto devices = capture->GetDevices();
    m_S33DeviceDetected = !devices.empty();
    if (!m_S33DeviceDetected)
    {
        m_S33LastResult = AudioCaptureResult::DeviceNotFound;
        return;
    }

    if (!m_S33SelectedDeviceId.empty())
    {
        const bool selectedDeviceExists =
            std::any_of(devices.begin(), devices.end(),
                        [this](const AudioCaptureDevice& device) { return device.id == m_S33SelectedDeviceId; });
        if (!selectedDeviceExists)
            m_S33SelectedDeviceId.clear();
    }

    if (capture->IsCapturing())
    {
        m_S33LastResult = AudioCaptureResult::AlreadyRunning;
        return;
    }
    if (!startIfAvailable)
        return;

    ApplyScenario33CaptureSettings();
    capture->BeginCalibration(m_S33CalibrationSeconds);
    m_S33LastResult = capture->Start(m_S33SelectedDeviceId);
    m_S33StartedCapture = m_S33LastResult == AudioCaptureResult::Success;
}

void SampleState::StopScenario33Capture()
{
    if (m_S33StartedCapture)
    {
        if (auto* capture = Resolve<IAudioCaptureService>())
            capture->Stop();
    }
    m_S33StartedCapture = false;
}

void SampleState::LoadScene33()
{
    auto& scene = GetScene();
    auto& resources = Get<ResourceManager>();

    EntityBuilder(scene, resources, "scenario")
        .WithName("MicrophoneStage")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(28.0f, 1.0f, 12.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.7f, 1.0f)
        .WithRendererColor(glm::vec4(0.035f, 0.05f, 0.08f, 1.0f))
        .Build();

    EntityBuilder(scene, resources, "scenario")
        .WithName("MicrophoneKeyLight")
        .WithTransform(glm::vec3(6.0f, 12.0f, 8.0f), glm::vec3(-45.0f, -25.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.45f, -1.0f, -0.35f)), glm::vec3(0.55f, 0.78f, 1.0f), 2.0f)
        .Build();

    m_S33VisualizerBars.clear();
    constexpr int barCount = 32;
    m_S33VisualizerBars.reserve(barCount);
    for (int index = 0; index < barCount; ++index)
    {
        const float x = (static_cast<float>(index) - (barCount - 1) * 0.5f) * 0.62f;
        const float t = static_cast<float>(index) / static_cast<float>(barCount - 1);
        Entity bar = EntityBuilder(scene, resources, "scenario")
                         .WithName("MicMeterBar_" + std::to_string(index))
                         .WithTransform(glm::vec3(x, 0.15f, 0.0f), glm::vec3(0.0f), glm::vec3(0.24f, 0.15f, 0.55f))
                         .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.35f, 1.0f)
                         .WithRendererColor(glm::vec4(0.1f + 0.75f * t, 0.8f - 0.25f * t, 1.0f - 0.8f * t, 1.0f))
                         .Build();
        m_S33VisualizerBars.push_back(bar);
    }

    m_S33Snapshot = {};
    m_S33History.fill(0.0f);
    m_S33HistoryOffset = 0;
    m_S33StartedCapture = false;

    if (auto* capture = Resolve<IAudioCaptureService>())
    {
        const auto settings = capture->GetSettings();
        m_S33InputVolume = settings.inputVolume;
        m_S33NoiseGate = settings.noiseGate;
        m_S33Gain = settings.gain;
        m_S33AttackSeconds = settings.attackSeconds;
        m_S33ReleaseSeconds = settings.releaseSeconds;
        m_S33PeakDecaySeconds = settings.peakDecaySeconds;
        m_S33CalibrationSeconds = settings.calibrationSeconds;
        m_S33PulseThreshold = settings.pulseThreshold;
        m_S33PulseCooldown = settings.pulseCooldown;
        m_S33PulseDuration = settings.pulseDuration;
    }
    if (auto* config = Resolve<ConfigManager>())
        m_S33SelectedDeviceId = config->GetConfig().audio.captureDevice;

    RefreshScenario33Capture(true);
}
