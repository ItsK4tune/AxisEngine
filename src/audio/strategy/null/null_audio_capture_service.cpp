#include <audio/strategy/null/null_audio_capture_service.h>

bool NullAudioCaptureService::Initialize(const AudioCaptureSettings& settings)
{
    m_Processor.SetSettings(settings);
    return true;
}

void NullAudioCaptureService::Shutdown()
{
    m_Processor.ResetLevelState();
}

bool NullAudioCaptureService::RefreshDevices()
{
    return true;
}

std::vector<AudioCaptureDevice> NullAudioCaptureService::GetDevices() const
{
    return {};
}

AudioCaptureResult NullAudioCaptureService::Start(const std::string&)
{
    return AudioCaptureResult::Unsupported;
}

void NullAudioCaptureService::Stop()
{
    m_Processor.ResetLevelState();
}

bool NullAudioCaptureService::IsCapturing() const
{
    return false;
}

void NullAudioCaptureService::Update(float deltaTime)
{
    m_Processor.Update(deltaTime, false, 0.0f, 0.0f);
}

void NullAudioCaptureService::BeginCalibration(float seconds)
{
    m_Processor.BeginCalibration(seconds);
}

void NullAudioCaptureService::SetSettings(const AudioCaptureSettings& settings)
{
    m_Processor.SetSettings(settings);
}

AudioCaptureSettings NullAudioCaptureService::GetSettings() const
{
    return m_Processor.GetSettings();
}

AudioCaptureSnapshot NullAudioCaptureService::GetSnapshot() const
{
    return m_Processor.GetSnapshot();
}
