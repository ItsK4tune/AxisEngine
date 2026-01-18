#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <glad/glad.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool Load(const std::string& filepath);
    void Unload();

    void Play();
    void Pause();
    void Stop();
    
    // Updates the internal texture if a new frame is ready
    void Update(float dt);

    unsigned int GetTextureID() const { return m_TextureID; }
    bool IsPlaying() const { return m_State == State::Playing; }
    bool IsLooping() const { return m_Loop; }
    void SetLoop(bool loop) { m_Loop = loop; }
    float GetSpeed() const { return m_Speed; }
    void SetSpeed(float speed) { m_Speed = speed; }

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    void Seek(double timestamp);

    void SetMaxDecodeSteps(int steps) { m_MaxDecodeSteps = steps; }
    int GetMaxDecodeSteps() const { return m_MaxDecodeSteps; }
    void SetOutputSize(int width, int height);

    double GetFrameRate() const { return m_FrameRate; }

private:
    void InitTexture();
    bool DecodeFrame();
    void UploadFrame();

    enum class State { Stopped, Playing, Paused };

    State m_State = State::Stopped;
    std::string m_Filepath;
    bool m_Loop = false;
    float m_Speed = 1.0f;
    int m_MaxDecodeSteps = 5;

    // FFmpeg context
    AVFormatContext* m_FormatCtx = nullptr;
    AVCodecContext* m_CodecCtx = nullptr;
    AVFrame* m_Frame = nullptr;
    AVFrame* m_RGBFrame = nullptr;
    SwsContext* m_SwsCtx = nullptr;
    int m_VideoStreamIndex = -1;

    unsigned int m_TextureID = 0;
    
    // Source Dimensions
    int m_Width = 0;
    int m_Height = 0;

    // Output Dimensions
    int m_OutputWidth = 0;
    int m_OutputHeight = 0;

    // Time keeping
    double m_CurrentTime = 0.0;
    double m_LastFrameTime = 0.0;
    double m_TimeBase = 0.0;
    double m_FrameRate = 0.0;
};
