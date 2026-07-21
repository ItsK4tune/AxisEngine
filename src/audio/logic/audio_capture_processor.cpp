#include <audio/logic/audio_capture_processor.h>
#include <algorithm>
#include <cmath>

AudioCaptureProcessor::AudioCaptureProcessor(const AudioCaptureSettings& settings)
    : m_Settings(SanitizeSettings(settings))
{
}

AudioCaptureSettings AudioCaptureProcessor::SanitizeSettings(AudioCaptureSettings settings)
{
    const auto finiteOr = [](float value, float fallback) { return std::isfinite(value) ? value : fallback; };
    settings.inputVolume = finiteOr(settings.inputVolume, 1.0f);
    settings.noiseGate = finiteOr(settings.noiseGate, 0.02f);
    settings.gain = finiteOr(settings.gain, 4.0f);
    settings.attackSeconds = finiteOr(settings.attackSeconds, 0.05f);
    settings.releaseSeconds = finiteOr(settings.releaseSeconds, 0.05f);
    settings.peakDecaySeconds = finiteOr(settings.peakDecaySeconds, 0.125f);
    settings.calibrationSeconds = finiteOr(settings.calibrationSeconds, 1.0f);
    settings.pulseThreshold = finiteOr(settings.pulseThreshold, 0.15f);
    settings.pulseCooldown = finiteOr(settings.pulseCooldown, 0.08f);
    settings.pulseDuration = finiteOr(settings.pulseDuration, 0.6f);
    settings.inputVolume = std::clamp(settings.inputVolume, 0.0f, 4.0f);
    settings.noiseGate = std::clamp(settings.noiseGate, 0.0f, 0.99f);
    settings.gain = std::max(settings.gain, 0.0f);
    settings.attackSeconds = std::clamp(settings.attackSeconds, 0.001f, 5.0f);
    settings.releaseSeconds = std::clamp(settings.releaseSeconds, 0.001f, 5.0f);
    settings.peakDecaySeconds = std::clamp(settings.peakDecaySeconds, 0.001f, 10.0f);
    settings.calibrationSeconds = std::max(settings.calibrationSeconds, 0.0f);
    settings.pulseThreshold = std::clamp(settings.pulseThreshold, 0.0f, 1.0f);
    settings.pulseCooldown = std::max(settings.pulseCooldown, 0.0f);
    settings.pulseDuration = std::max(settings.pulseDuration, 0.001f);
    return settings;
}

void AudioCaptureProcessor::SetSettings(const AudioCaptureSettings& settings)
{
    m_Settings = SanitizeSettings(settings);
}

const AudioCaptureSettings& AudioCaptureProcessor::GetSettings() const
{
    return m_Settings;
}

void AudioCaptureProcessor::BeginCalibration(float seconds)
{
    m_CalibrationRemaining = std::max(seconds, 0.0f);
    m_CalibrationWeightedTotal = 0.0f;
    m_CalibrationElapsed = 0.0f;
    m_Snapshot.level.noiseFloor = 0.0f;
}

void AudioCaptureProcessor::SetPulseOrigin(const glm::vec3& origin)
{
    if (std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z))
        m_PulseOrigin = origin;
}

void AudioCaptureProcessor::Update(float deltaTime, bool isCapturing, float rawRms, float rawPeak)
{
    const float dt = std::max(deltaTime, 0.0f);
    rawRms = isCapturing ? std::clamp(rawRms * m_Settings.inputVolume, 0.0f, 1.0f) : 0.0f;
    rawPeak = isCapturing ? std::clamp(rawPeak * m_Settings.inputVolume, 0.0f, 1.0f) : 0.0f;

    auto& snapshot = m_Snapshot;
    const float responseSeconds = rawRms > snapshot.level.rms ? m_Settings.attackSeconds : m_Settings.releaseSeconds;
    const float smoothing = 1.0f - std::exp(-dt / responseSeconds);
    snapshot.level.rms += (rawRms - snapshot.level.rms) * smoothing;
    snapshot.level.peak = std::max(rawPeak, snapshot.level.peak * std::exp(-dt / m_Settings.peakDecaySeconds));

    if (isCapturing && m_CalibrationRemaining > 0.0f && dt > 0.0f)
    {
        const float calibrationStep = std::min(dt, m_CalibrationRemaining);
        m_CalibrationWeightedTotal += rawRms * calibrationStep;
        m_CalibrationElapsed += calibrationStep;
        m_CalibrationRemaining = std::max(0.0f, m_CalibrationRemaining - dt);
        if (m_CalibrationRemaining == 0.0f && m_CalibrationElapsed > 0.0f)
            snapshot.level.noiseFloor = m_CalibrationWeightedTotal / m_CalibrationElapsed;
    }

    const float gate = std::max(m_Settings.noiseGate, snapshot.level.noiseFloor * 1.5f);
    const float gated = std::max(0.0f, snapshot.level.rms - gate);
    snapshot.level.intensity = std::clamp(gated * m_Settings.gain / std::max(1.0f - gate, 0.001f), 0.0f, 1.0f);

    m_PulseCooldownRemaining = std::max(0.0f, m_PulseCooldownRemaining - dt);
    for (auto& pulse : snapshot.pulses) pulse.age += dt;
    std::erase_if(snapshot.pulses, [](const AudioPulse& pulse) { return pulse.age >= pulse.duration; });

    if (isCapturing && m_CalibrationRemaining == 0.0f && snapshot.level.intensity >= m_Settings.pulseThreshold &&
        m_PulseCooldownRemaining == 0.0f)
    {
        AudioPulse pulse;
        pulse.origin = m_PulseOrigin;
        pulse.intensity = snapshot.level.intensity;
        pulse.peak = snapshot.level.peak;
        pulse.duration = m_Settings.pulseDuration;
        snapshot.pulses.push_back(pulse);
        if (snapshot.pulses.size() > AudioCaptureLimits::MaxPulses)
        {
            snapshot.pulses.erase(snapshot.pulses.begin(),
                                  snapshot.pulses.begin() + static_cast<std::ptrdiff_t>(snapshot.pulses.size() -
                                                                                        AudioCaptureLimits::MaxPulses));
        }
        m_PulseCooldownRemaining = m_Settings.pulseCooldown;
    }
}

void AudioCaptureProcessor::ResetLevelState()
{
    const float noiseFloor = m_Snapshot.level.noiseFloor;
    m_Snapshot = {};
    m_Snapshot.level.noiseFloor = noiseFloor;
    m_PulseCooldownRemaining = 0.0f;
}

const AudioCaptureSnapshot& AudioCaptureProcessor::GetSnapshot() const
{
    return m_Snapshot;
}
