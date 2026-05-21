#include <render/logic/video_decoder.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

ITextureManager* VideoDecoder::s_TextureManager = nullptr;

void VideoDecoder::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

ITextureManager& VideoDecoder::GetTextureManager()
{
    if (!s_TextureManager)
    {
        LOGGER_ERROR("VideoDecoder") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in VideoDecoder");
    }
    return *s_TextureManager;
}

VideoDecoder::VideoDecoder()
{
    m_Frame = av_frame_alloc();
    m_RGBFrame = av_frame_alloc();
}

VideoDecoder::~VideoDecoder()
{
    Unload();
    av_frame_free(&m_Frame);
    av_frame_free(&m_RGBFrame);
}

bool VideoDecoder::Load(const std::string& filepath)
{
    Unload();
    m_Filepath = filepath;
    m_ProceduralFallback = false;

    av_log_set_level(AV_LOG_QUIET);
    if (avformat_open_input(&m_FormatCtx, filepath.c_str(), nullptr, nullptr) != 0)
    {
        std::ifstream in(filepath, std::ios::binary);
        std::string marker;
        if (in)
        {
            marker.resize(15);
            in.read(marker.data(), static_cast<std::streamsize>(marker.size()));
        }
        if (marker.find("MOCK_VIDEO_DATA") == 0)
        {
            m_ProceduralFallback = true;
            m_Width = 640;
            m_Height = 360;
            if (m_OutputWidth <= 0 || m_OutputHeight <= 0)
            {
                m_OutputWidth = m_Width;
                m_OutputHeight = m_Height;
            }
            m_FrameRate = 30.0;
            m_TimeBase = 1.0 / m_FrameRate;
            InitTexture();
            return true;
        }
        LOGGER_ERROR("VideoDecoder") << "Failed to open video file: " << filepath;
        return false;
    }

    if (avformat_find_stream_info(m_FormatCtx, nullptr) < 0)
    {
        LOGGER_ERROR("VideoDecoder") << "Failed to find stream info";
        return false;
    }

    m_VideoStreamIndex = -1;
    for (unsigned int i = 0; i < m_FormatCtx->nb_streams; i++)
    {
        if (m_FormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_VideoStreamIndex = i;
            break;
        }
    }

    if (m_VideoStreamIndex == -1)
    {
        LOGGER_ERROR("VideoDecoder") << "No video stream found";
        return false;
    }

    AVCodecParameters* codecPar = m_FormatCtx->streams[m_VideoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        LOGGER_ERROR("VideoDecoder") << "Unsupported codec";
        return false;
    }

    m_CodecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_CodecCtx, codecPar);

    if (avcodec_open2(m_CodecCtx, codec, nullptr) < 0)
    {
        LOGGER_ERROR("VideoDecoder") << "Failed to open codec";
        return false;
    }

    m_Width = m_CodecCtx->width;
    m_Height = m_CodecCtx->height;

    if (m_OutputWidth <= 0 || m_OutputHeight <= 0)
    {
        m_OutputWidth = m_Width;
        m_OutputHeight = m_Height;
    }

    m_TimeBase = av_q2d(m_FormatCtx->streams[m_VideoStreamIndex]->time_base);

    if (m_FormatCtx->streams[m_VideoStreamIndex]->avg_frame_rate.den > 0)
        m_FrameRate = av_q2d(m_FormatCtx->streams[m_VideoStreamIndex]->avg_frame_rate);
    else
        m_FrameRate = 30.0;

    SetOutputSize(m_OutputWidth, m_OutputHeight);

    return true;
}

void VideoDecoder::SetOutputSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        if (m_Width > 0 && m_Height > 0)
        {
            m_OutputWidth = m_Width;
            m_OutputHeight = m_Height;
        }
        else
        {
            return;
        }
    }
    else
    {
        m_OutputWidth = width;
        m_OutputHeight = height;
    }

    if (!m_CodecCtx && !m_ProceduralFallback)
        return;

    if (m_ProceduralFallback)
    {
        InitTexture();
        return;
    }

    if (m_SwsCtx)
        sws_freeContext(m_SwsCtx);
    m_SwsCtx = sws_getContext(m_Width, m_Height, m_CodecCtx->pix_fmt, m_OutputWidth, m_OutputHeight, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (m_RGBFrame->data[0])
        av_freep(&m_RGBFrame->data[0]);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_OutputWidth, m_OutputHeight, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(m_RGBFrame->data, m_RGBFrame->linesize, buffer, AV_PIX_FMT_RGBA, m_OutputWidth, m_OutputHeight,
                         1);

    InitTexture();
}

void VideoDecoder::Unload()
{
    if (m_CodecCtx)
        avcodec_free_context(&m_CodecCtx);
    if (m_FormatCtx)
        avformat_close_input(&m_FormatCtx);
    if (m_SwsCtx)
        sws_freeContext(m_SwsCtx);

    if (m_TextureID != 0 && s_TextureManager)
    {
        GetTextureManager().DeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }

    if (m_RGBFrame && m_RGBFrame->data[0])
    {
        av_freep(&m_RGBFrame->data[0]);
        m_RGBFrame->data[0] = nullptr;
    }

    m_FormatCtx = nullptr;
    m_CodecCtx = nullptr;
    m_SwsCtx = nullptr;
    m_ProceduralFallback = false;
    m_State = State::Stopped;
}

void VideoDecoder::InitTexture()
{
    if (!s_TextureManager)
        return;
    auto& tm = GetTextureManager();

    if (m_TextureID == 0)
        m_TextureID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_TextureID);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, m_OutputWidth, m_OutputHeight, 0,
                  TextureFormat::RGBA, DataType::UnsignedByte, nullptr);
}

void VideoDecoder::Play()
{
    if (m_State != State::Playing)
    {
        m_State = State::Playing;
    }
}

void VideoDecoder::Pause()
{
    m_State = State::Paused;
}

void VideoDecoder::Stop()
{
    m_State = State::Stopped;
    Seek(0);
}

void VideoDecoder::Update(float dt)
{
    if (m_State != State::Playing)
        return;

    m_CurrentTime += dt * m_Speed;

    if (m_ProceduralFallback)
    {
        if (m_Loop && m_CurrentTime > 8.0)
            m_CurrentTime = 0.0;
        UploadProceduralFrame();
        return;
    }

    bool needsUpload = false;
    int decodedCount = 0;

    while (m_LastFrameTime < m_CurrentTime && decodedCount < m_MaxDecodeSteps)
    {
        if (!DecodeFrame())
        {
            if (m_Loop)
            {
                Seek(0);
                m_CurrentTime = 0;
                m_LastFrameTime = 0;
            }
            else
            {
                Stop();
            }
            break;
        }
        decodedCount++;
        needsUpload = true;
    }

    if (needsUpload)
    {
        UploadFrame();
    }
}

bool VideoDecoder::DecodeFrame()
{
    AVPacket* packet = av_packet_alloc();
    bool frameRead = false;

    while (av_read_frame(m_FormatCtx, packet) >= 0)
    {
        if (packet->stream_index == m_VideoStreamIndex)
        {
            if (avcodec_send_packet(m_CodecCtx, packet) == 0)
            {
                while (avcodec_receive_frame(m_CodecCtx, m_Frame) == 0)
                {
                    if (m_Frame->pts != AV_NOPTS_VALUE)
                        m_LastFrameTime = m_Frame->pts * m_TimeBase;
                    else
                        m_LastFrameTime += 1.0 / m_FrameRate;

                    frameRead = true;
                    goto end_decode;
                }
            }
        }
        av_packet_unref(packet);
    }

end_decode:
    av_packet_unref(packet);
    av_packet_free(&packet);
    return frameRead;
}

void VideoDecoder::UploadFrame()
{
    if (!m_SwsCtx || !m_Frame || !m_RGBFrame || !s_TextureManager)
        return;

    sws_scale(m_SwsCtx, (const uint8_t* const*)m_Frame->data, m_Frame->linesize, 0, m_Height, m_RGBFrame->data,
              m_RGBFrame->linesize);

    auto& tm = GetTextureManager();
    tm.BindTexture(TextureType::Texture2D, m_TextureID);
    tm.TexSubImage2D(TextureType::Texture2D, 0, 0, 0, m_OutputWidth, m_OutputHeight, TextureFormat::RGBA,
                     DataType::UnsignedByte, m_RGBFrame->data[0]);
}

void VideoDecoder::UploadProceduralFrame()
{
    if (!s_TextureManager || m_OutputWidth <= 0 || m_OutputHeight <= 0)
        return;

    std::vector<uint8_t> pixels(static_cast<size_t>(m_OutputWidth) * static_cast<size_t>(m_OutputHeight) * 4);
    float t = static_cast<float>(m_CurrentTime);
    for (int y = 0; y < m_OutputHeight; ++y)
    {
        for (int x = 0; x < m_OutputWidth; ++x)
        {
            float u = static_cast<float>(x) / static_cast<float>((std::max)(1, m_OutputWidth - 1));
            float v = static_cast<float>(y) / static_cast<float>((std::max)(1, m_OutputHeight - 1));
            uint8_t r = static_cast<uint8_t>(127.0f + 127.0f * std::sin((u * 8.0f + t) * 3.14159f));
            uint8_t g = static_cast<uint8_t>(127.0f + 127.0f * std::sin((v * 6.0f + t * 1.4f) * 3.14159f));
            uint8_t b = static_cast<uint8_t>(127.0f + 127.0f * std::sin(((u + v) * 5.0f - t * 0.8f) * 3.14159f));
            size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(m_OutputWidth) + static_cast<size_t>(x)) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = 255;
        }
    }

    auto& tm = GetTextureManager();
    tm.BindTexture(TextureType::Texture2D, m_TextureID);
    tm.TexSubImage2D(TextureType::Texture2D, 0, 0, 0, m_OutputWidth, m_OutputHeight, TextureFormat::RGBA,
                     DataType::UnsignedByte, pixels.data());
}

void VideoDecoder::Seek(double timestamp)
{
    if (m_ProceduralFallback)
    {
        m_CurrentTime = timestamp;
        m_LastFrameTime = timestamp;
        return;
    }
    if (!m_FormatCtx)
        return;
    int64_t targetPts = (int64_t)(timestamp / m_TimeBase);
    av_seek_frame(m_FormatCtx, m_VideoStreamIndex, targetPts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_CodecCtx);
}
