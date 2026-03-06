#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class ITextureManager;

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

    unsigned int GetTextureID() const { return m_TextureID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    double GetDuration() const { return m_FormatCtx ? (double)m_FormatCtx->duration / AV_TIME_BASE : 0.0; }
    double GetCurrentTime() const { return m_CurrentTime; }
    bool IsPlaying() const { return m_State == State::Playing; }

    void SetOutputSize(int width, int height);

    void SetLoop(bool loop) { m_Loop = loop; }
    bool IsLooping() const { return m_Loop; }

    float GetSpeed() const { return (float)m_Speed; }
    void SetSpeed(float speed) { m_Speed = speed; }

    int GetMaxDecodeSteps() const { return m_MaxDecodeSteps; }
    void SetMaxDecodeSteps(int steps) { m_MaxDecodeSteps = steps; }

    static void SetTextureManager(ITextureManager& textureManager);

private:
    AVFormatContext* m_FormatCtx = nullptr;
    AVCodecContext* m_CodecCtx = nullptr;
    SwsContext* m_SwsCtx = nullptr;
    AVFrame* m_Frame = nullptr;
    AVFrame* m_RGBFrame = nullptr;
    int m_VideoStreamIndex = -1;

    unsigned int m_TextureID = 0;
    int m_Width = 0;
    int m_Height = 0;
    int m_OutputWidth = 0;
    int m_OutputHeight = 0;

    double m_TimeBase = 0.0;
    double m_FrameRate = 0.0;
    double m_CurrentTime = 0.0;
    double m_LastFrameTime = 0.0;
    double m_Limit = 0.0;
    double m_Speed = 1.0;
    bool m_Loop = true;

    enum class State { Stopped, Playing, Paused };
    State m_State = State::Stopped;

    std::string m_Filepath;

    int m_MaxDecodeSteps = 5;

    static ITextureManager* s_TextureManager;
    static ITextureManager& GetTextureManager();

    bool DecodeFrame();
    void UploadFrame();
    void InitTexture();
};
