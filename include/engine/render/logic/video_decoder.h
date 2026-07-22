#pragma once

#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <mutex>
#include <condition_variable>
#include <deque>
#include <string>
#include <thread>
#include <vector>

class ITextureManager;
class IBufferManager;

class VideoDecoder
{
public:
    VideoDecoder();
    ~VideoDecoder();

    bool Load(const std::string& filepath);
    void Unload();

    void Play();
    void Pause();
    void Stop();
    void Seek(double timestamp);

    void Update(float dt);

    unsigned int GetTextureID() const
    {
        return m_TextureID;
    }
    int GetWidth() const
    {
        return m_Width;
    }
    int GetHeight() const
    {
        return m_Height;
    }
    double GetDuration() const
    {
        if (m_ProceduralFallback)
            return 8.0;
        return m_FormatCtx ? (double)m_FormatCtx->duration / AV_TIME_BASE : 0.0;
    }
    double GetCurrentTime() const
    {
        return m_CurrentTime;
    }
    bool IsPlaying() const
    {
        return m_State.load(std::memory_order_acquire) == State::Playing;
    }
    bool HasAudioStream() const { return m_AudioStreamIndex >= 0; }

    void SetOutputSize(int width, int height);

    void SetLoop(bool loop)
    {
        m_Loop.store(loop, std::memory_order_release);
    }
    bool IsLooping() const
    {
        return m_Loop.load(std::memory_order_acquire);
    }

    float GetSpeed() const
    {
        return (float)m_Speed;
    }
    void SetSpeed(float speed)
    {
        m_Speed = speed;
    }

    int GetMaxDecodeSteps() const
    {
        return m_MaxDecodeSteps;
    }
    void SetMaxDecodeSteps(int steps)
    {
        m_MaxDecodeSteps = steps;
    }
    void SetAsyncDecodeEnabled(bool enabled)
    {
        const bool previous = m_AsyncDecodeEnabled.exchange(enabled, std::memory_order_acq_rel);
        if (previous == enabled)
            return;
        if (!enabled)
            StopDecodeWorker();
        else if (IsPlaying())
        {
            StartDecodeWorker();
            m_DecodeCondition.notify_all();
        }
    }
    bool IsAsyncDecodeEnabled() const { return m_AsyncDecodeEnabled.load(std::memory_order_acquire); }
    void SetMaxQueuedFrames(size_t frames)
    {
        m_MaxQueuedFrames.store((std::max)(size_t{1}, frames), std::memory_order_release);
        m_DecodeCondition.notify_all();
    }

    static void SetTextureManager(ITextureManager& textureManager);
    static void SetBufferManager(IBufferManager& bufferManager);
    static void ClearTextureManager();
    static void ClearBufferManager();

private:
    AVFormatContext* m_FormatCtx = nullptr;
    AVCodecContext* m_CodecCtx = nullptr;
    SwsContext* m_SwsCtx = nullptr;
    AVFrame* m_Frame = nullptr;
    AVPacket* m_Packet = nullptr;
    int m_VideoStreamIndex = -1;
    int m_AudioStreamIndex = -1;

    unsigned int m_TextureID = 0;
    int m_Width = 0;
    int m_Height = 0;
    int m_OutputWidth = 0;
    int m_OutputHeight = 0;

    double m_TimeBase = 0.0;
    double m_FrameRate = 0.0;
    double m_CurrentTime = 0.0;
    double m_LastFrameTime = 0.0;
    double m_Speed = 1.0;
    std::atomic<bool> m_Loop{true};
    bool m_ProceduralFallback = false;

    enum class State
    {
        Stopped,
        Playing,
        Paused
    };
    std::atomic<State> m_State{State::Stopped};

    std::string m_Filepath;

    int m_MaxDecodeSteps = 5;

    struct DecodedFrame
    {
        std::vector<uint8_t> pixels;
        double timestamp = 0.0;
    };
    std::thread m_DecodeThread;
    std::mutex m_DecodeMutex;
    std::condition_variable m_DecodeCondition;
    std::deque<DecodedFrame> m_DecodedFrames;
    std::atomic<bool> m_StopDecodeThread{false};
    std::atomic<bool> m_AsyncDecodeEnabled{true};
    std::atomic<bool> m_DecodeReachedEnd{false};
    bool m_SeekRequested = false;
    double m_RequestedSeekTime = 0.0;
    std::vector<uint8_t> m_UploadPixels;
    unsigned int m_UploadPbos[2] = {0, 0};
    size_t m_UploadPboCapacities[2] = {0, 0};
    size_t m_UploadPboIndex = 0;
    std::atomic<size_t> m_MaxQueuedFrames{3};

    static ITextureManager* s_TextureManager;
    static IBufferManager* s_BufferManager;
    static ITextureManager& GetTextureManager();

    bool DecodeFrame(DecodedFrame& output);
    void UploadFrame(const std::vector<uint8_t>& pixels);
    void StartDecodeWorker();
    void StopDecodeWorker();
    void DecodeWorkerLoop();
    void UploadProceduralFrame();
    void InitTexture();
};
