#include <platform/strategy/windows/wasapi_audio_capture_service.h>

#if defined(_WIN32)

#include <audio/logic/audio_capture_processor.h>
#include <core/logic/logger.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace
{
class ScopedCOM
{
public:
    ScopedCOM()
    {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        m_Initialized = result == S_OK || result == S_FALSE;
        m_Usable = m_Initialized || result == RPC_E_CHANGED_MODE;
    }

    ~ScopedCOM()
    {
        if (m_Initialized)
            CoUninitialize();
    }

    bool IsUsable() const
    {
        return m_Usable;
    }

private:
    bool m_Initialized = false;
    bool m_Usable = false;
};

std::string ToUtf8(const wchar_t* text)
{
    if (!text || !*text)
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1)
        return {};
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

std::wstring ToWide(const std::string& text)
{
    if (text.empty())
        return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 1)
        return {};
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

float ReadSample(const BYTE* data, uint32_t sampleIndex, const WAVEFORMATEX& format, bool isFloat, uint16_t validBits)
{
    const uint32_t bytesPerSample = format.wBitsPerSample / 8;
    const BYTE* sample = data + static_cast<size_t>(sampleIndex) * bytesPerSample;
    if (isFloat && format.wBitsPerSample == 32)
    {
        float value = 0.0f;
        std::memcpy(&value, sample, sizeof(value));
        return value;
    }
    if (isFloat && format.wBitsPerSample == 64)
    {
        double value = 0.0;
        std::memcpy(&value, sample, sizeof(value));
        return static_cast<float>(value);
    }

    switch (format.wBitsPerSample)
    {
        case 8:
            return (static_cast<float>(*sample) - 128.0f) / 128.0f;
        case 16: {
            int16_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            return static_cast<float>(value) / 32768.0f;
        }
        case 24: {
            int32_t value = static_cast<int32_t>(sample[0]) | (static_cast<int32_t>(sample[1]) << 8) |
                            (static_cast<int32_t>(sample[2]) << 16);
            if (value & 0x00800000)
                value |= static_cast<int32_t>(0xFF000000);
            return static_cast<float>(value) / 8388608.0f;
        }
        case 32: {
            int32_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            if (validBits > 0 && validBits < 32)
                value >>= (32 - validBits);
            const auto denominator = std::ldexp(1.0, (validBits > 0 ? validBits : 32) - 1);
            return static_cast<float>(static_cast<double>(value) / denominator);
        }
        default:
            return 0.0f;
    }
}
}  // namespace

struct WASAPIAudioCaptureService::Impl
{
    mutable std::mutex stateMutex;
    mutable std::mutex devicesMutex;
    AudioCaptureProcessor processor;
    std::vector<AudioCaptureDevice> devices;
    std::jthread captureThread;
    std::atomic<bool> capturing = false;
    std::mutex accumulatorMutex;
    double accumulatedSquareSum = 0.0;
    uint64_t accumulatedSampleCount = 0;
    float accumulatedPeak = 0.0f;

    void AccumulateSamples(double squareSum, uint64_t sampleCount, float peak)
    {
        std::lock_guard lock(accumulatorMutex);
        accumulatedSquareSum += squareSum;
        accumulatedSampleCount += sampleCount;
        accumulatedPeak = (std::max)(accumulatedPeak, peak);
    }

    bool EnumerateDevices()
    {
        ScopedCOM com;
        if (!com.IsUsable())
            return false;

        ComPtr<IMMDeviceEnumerator> enumerator;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                    IID_PPV_ARGS(enumerator.GetAddressOf()))))
            return false;

        std::wstring defaultId;
        ComPtr<IMMDevice> defaultDevice;
        if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, defaultDevice.GetAddressOf())))
        {
            LPWSTR id = nullptr;
            if (SUCCEEDED(defaultDevice->GetId(&id)))
            {
                defaultId = id;
                CoTaskMemFree(id);
            }
        }

        ComPtr<IMMDeviceCollection> collection;
        if (FAILED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, collection.GetAddressOf())))
            return false;

        UINT count = 0;
        collection->GetCount(&count);
        std::vector<AudioCaptureDevice> found;
        found.reserve(count);
        for (UINT i = 0; i < count; ++i)
        {
            ComPtr<IMMDevice> device;
            if (FAILED(collection->Item(i, device.GetAddressOf())))
                continue;

            LPWSTR rawId = nullptr;
            if (FAILED(device->GetId(&rawId)))
                continue;

            AudioCaptureDevice info;
            info.id = ToUtf8(rawId);
            info.isDefault = defaultId == rawId;
            CoTaskMemFree(rawId);

            ComPtr<IPropertyStore> properties;
            PROPVARIANT name;
            PropVariantInit(&name);
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, properties.GetAddressOf())) &&
                SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &name)) && name.vt == VT_LPWSTR)
                info.name = ToUtf8(name.pwszVal);
            PropVariantClear(&name);
            if (info.name.empty())
                info.name = info.id;
            found.push_back(std::move(info));
        }

        std::scoped_lock lock(devicesMutex);
        devices = std::move(found);
        return true;
    }

    void Capture(std::stop_token stopToken, std::wstring requestedId, std::promise<AudioCaptureResult> startupPromise)
    {
        ScopedCOM com;
        if (!com.IsUsable())
        {
            startupPromise.set_value(AudioCaptureResult::BackendError);
            return;
        }

        ComPtr<IMMDeviceEnumerator> enumerator;
        HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                          IID_PPV_ARGS(enumerator.GetAddressOf()));
        if (FAILED(result))
        {
            startupPromise.set_value(AudioCaptureResult::BackendError);
            return;
        }

        ComPtr<IMMDevice> device;
        result = requestedId.empty() ? enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, device.GetAddressOf())
                                     : enumerator->GetDevice(requestedId.c_str(), device.GetAddressOf());
        if (FAILED(result))
        {
            startupPromise.set_value(AudioCaptureResult::DeviceNotFound);
            return;
        }

        ComPtr<IAudioClient> audioClient;
        result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void**>(audioClient.GetAddressOf()));
        if (FAILED(result))
        {
            startupPromise.set_value(result == E_ACCESSDENIED ? AudioCaptureResult::PermissionDenied
                                                              : AudioCaptureResult::BackendError);
            return;
        }

        WAVEFORMATEX* format = nullptr;
        if (FAILED(audioClient->GetMixFormat(&format)) || !format)
        {
            startupPromise.set_value(AudioCaptureResult::BackendError);
            return;
        }

        const bool extensible = format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                                format->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        const auto* extended = extensible ? reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format) : nullptr;
        const bool isFloat = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                             (extended && IsEqualGUID(extended->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));
        const bool isPcm = format->wFormatTag == WAVE_FORMAT_PCM ||
                           (extended && IsEqualGUID(extended->SubFormat, KSDATAFORMAT_SUBTYPE_PCM));
        const uint16_t validBits = extended && extended->Samples.wValidBitsPerSample != 0
                                       ? extended->Samples.wValidBitsPerSample
                                       : format->wBitsPerSample;
        const bool supportedSampleFormat =
            (isFloat && (format->wBitsPerSample == 32 || format->wBitsPerSample == 64)) ||
            (isPcm && (format->wBitsPerSample == 8 || format->wBitsPerSample == 16 || format->wBitsPerSample == 24 ||
                       format->wBitsPerSample == 32));
        if ((!isFloat && !isPcm) || !supportedSampleFormat || format->nChannels == 0)
        {
            CoTaskMemFree(format);
            startupPromise.set_value(AudioCaptureResult::Unsupported);
            return;
        }

        HANDLE captureEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!captureEvent)
        {
            CoTaskMemFree(format);
            startupPromise.set_value(AudioCaptureResult::BackendError);
            return;
        }

        result =
            audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, format, nullptr);
        if (SUCCEEDED(result))
            result = audioClient->SetEventHandle(captureEvent);

        ComPtr<IAudioCaptureClient> captureClient;
        if (SUCCEEDED(result))
            result = audioClient->GetService(IID_PPV_ARGS(captureClient.GetAddressOf()));
        if (SUCCEEDED(result))
            result = audioClient->Start();

        if (FAILED(result))
        {
            CloseHandle(captureEvent);
            CoTaskMemFree(format);
            startupPromise.set_value(result == E_ACCESSDENIED ? AudioCaptureResult::PermissionDenied
                                                              : AudioCaptureResult::BackendError);
            return;
        }

        capturing.store(true, std::memory_order_release);
        startupPromise.set_value(AudioCaptureResult::Success);

        bool captureFailed = false;
        while (!stopToken.stop_requested() && !captureFailed)
        {
            const DWORD waitResult = WaitForSingleObject(captureEvent, 100);
            if (waitResult == WAIT_TIMEOUT)
                continue;
            if (waitResult != WAIT_OBJECT_0)
                break;

            UINT32 packetFrames = 0;
            HRESULT packetResult = captureClient->GetNextPacketSize(&packetFrames);
            while (SUCCEEDED(packetResult) && packetFrames > 0)
            {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                if (FAILED(captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
                {
                    captureFailed = true;
                    break;
                }

                double squareSum = 0.0;
                float peak = 0.0f;
                const uint32_t sampleCount = frames * format->nChannels;
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data)
                {
                    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
                    {
                        const float value =
                            std::clamp(ReadSample(data, sampleIndex, *format, isFloat, validBits), -1.0f, 1.0f);
                        squareSum += static_cast<double>(value) * value;
                        peak = std::max(peak, std::abs(value));
                    }
                }
                AccumulateSamples(squareSum, sampleCount, peak);
                captureClient->ReleaseBuffer(frames);
                packetResult = captureClient->GetNextPacketSize(&packetFrames);
            }
            if (FAILED(packetResult))
                captureFailed = true;
        }

        if (captureFailed && !stopToken.stop_requested())
            LOGGER_WARN("AudioCapture") << "WASAPI capture stream stopped after a device or buffer error.";

        audioClient->Stop();
        capturing.store(false, std::memory_order_release);
        CloseHandle(captureEvent);
        CoTaskMemFree(format);
    }
};

WASAPIAudioCaptureService::WASAPIAudioCaptureService() : m_Impl(std::make_unique<Impl>())
{
}

WASAPIAudioCaptureService::~WASAPIAudioCaptureService()
{
    Shutdown();
}

bool WASAPIAudioCaptureService::Initialize(const AudioCaptureSettings& settings)
{
    SetSettings(settings);
    BeginCalibration(settings.calibrationSeconds);
    if (!RefreshDevices())
        LOGGER_WARN("AudioCapture") << "WASAPI initialized, but capture devices could not be enumerated.";
    return true;
}

bool WASAPIAudioCaptureService::RefreshDevices()
{
    return m_Impl->EnumerateDevices();
}

void WASAPIAudioCaptureService::Shutdown()
{
    Stop();
    std::scoped_lock lock(m_Impl->stateMutex);
    m_Impl->processor.ResetLevelState();
}

std::vector<AudioCaptureDevice> WASAPIAudioCaptureService::GetDevices() const
{
    std::scoped_lock lock(m_Impl->devicesMutex);
    return m_Impl->devices;
}

AudioCaptureResult WASAPIAudioCaptureService::Start(const std::string& deviceId)
{
    if (IsCapturing())
        return AudioCaptureResult::AlreadyRunning;
    if (m_Impl->captureThread.joinable())
        m_Impl->captureThread.join();

    const std::wstring wideDeviceId = ToWide(deviceId);
    if (!deviceId.empty() && wideDeviceId.empty())
        return AudioCaptureResult::DeviceNotFound;

    std::promise<AudioCaptureResult> startupPromise;
    auto startupResult = startupPromise.get_future();
    m_Impl->captureThread =
        std::jthread([impl = m_Impl.get(), id = wideDeviceId, promise = std::move(startupPromise)](
                         std::stop_token token) mutable { impl->Capture(token, std::move(id), std::move(promise)); });

    if (startupResult.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
    {
        Stop();
        return AudioCaptureResult::BackendError;
    }
    const AudioCaptureResult result = startupResult.get();
    if (result != AudioCaptureResult::Success)
        Stop();
    return result;
}

void WASAPIAudioCaptureService::Stop()
{
    if (m_Impl->captureThread.joinable())
    {
        m_Impl->captureThread.request_stop();
        m_Impl->captureThread.join();
    }
    m_Impl->capturing.store(false, std::memory_order_release);
    {
        std::lock_guard accumulatorLock(m_Impl->accumulatorMutex);
        m_Impl->accumulatedSquareSum = 0.0;
        m_Impl->accumulatedSampleCount = 0;
        m_Impl->accumulatedPeak = 0.0f;
    }
    std::scoped_lock lock(m_Impl->stateMutex);
    m_Impl->processor.ResetLevelState();
}

bool WASAPIAudioCaptureService::IsCapturing() const
{
    return m_Impl->capturing.load(std::memory_order_acquire);
}

void WASAPIAudioCaptureService::Update(float deltaTime)
{
    std::scoped_lock lock(m_Impl->stateMutex);
    const bool isCapturing = IsCapturing();
    double squareSum = 0.0;
    uint64_t sampleCount = 0;
    float rawPeak = 0.0f;
    {
        std::lock_guard accumulatorLock(m_Impl->accumulatorMutex);
        squareSum = std::exchange(m_Impl->accumulatedSquareSum, 0.0);
        sampleCount = std::exchange(m_Impl->accumulatedSampleCount, uint64_t{0});
        rawPeak = std::exchange(m_Impl->accumulatedPeak, 0.0f);
    }
    const float rawRms = isCapturing && sampleCount > 0
                             ? static_cast<float>(std::sqrt(squareSum / static_cast<double>(sampleCount)))
                             : 0.0f;
    m_Impl->processor.Update(deltaTime, isCapturing, rawRms, isCapturing ? rawPeak : 0.0f);
}

void WASAPIAudioCaptureService::SetPulseOrigin(const glm::vec3& origin)
{
    std::scoped_lock lock(m_Impl->stateMutex);
    m_Impl->processor.SetPulseOrigin(origin);
}

void WASAPIAudioCaptureService::BeginCalibration(float seconds)
{
    std::scoped_lock lock(m_Impl->stateMutex);
    m_Impl->processor.BeginCalibration(seconds);
}

void WASAPIAudioCaptureService::SetSettings(const AudioCaptureSettings& settings)
{
    std::scoped_lock lock(m_Impl->stateMutex);
    m_Impl->processor.SetSettings(settings);
}

AudioCaptureSettings WASAPIAudioCaptureService::GetSettings() const
{
    std::scoped_lock lock(m_Impl->stateMutex);
    return m_Impl->processor.GetSettings();
}

AudioCaptureSnapshot WASAPIAudioCaptureService::GetSnapshot() const
{
    std::scoped_lock lock(m_Impl->stateMutex);
    return m_Impl->processor.GetSnapshot();
}

#endif
