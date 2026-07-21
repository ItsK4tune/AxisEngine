#include <render/logic/video_decoder.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>

#ifndef AXIS_ENABLE_MOCK_VIDEO_DATA
#define AXIS_ENABLE_MOCK_VIDEO_DATA 0
#endif

ITextureManager* VideoDecoder::s_TextureManager = nullptr;
IBufferManager* VideoDecoder::s_BufferManager = nullptr;

void VideoDecoder::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

void VideoDecoder::ClearTextureManager()
{
    s_TextureManager = nullptr;
}

void VideoDecoder::SetBufferManager(IBufferManager& bufferManager)
{
    s_BufferManager = &bufferManager;
}

void VideoDecoder::ClearBufferManager()
{
    s_BufferManager = nullptr;
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
    m_Packet = av_packet_alloc();
}

VideoDecoder::~VideoDecoder()
{
    Unload();
    av_packet_free(&m_Packet);
    av_frame_free(&m_Frame);
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
#if AXIS_ENABLE_MOCK_VIDEO_DATA
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
#else
            LOGGER_ERROR("VideoDecoder") << "MOCK_VIDEO_DATA fallback is disabled for this build: " << filepath;
            return false;
#endif
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
    const bool restartWorker = m_DecodeThread.joinable() && IsPlaying();
    if (m_DecodeThread.joinable())
        StopDecodeWorker();
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

    InitTexture();
    if (restartWorker)
        StartDecodeWorker();
}

void VideoDecoder::Unload()
{
    StopDecodeWorker();
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

    if (s_BufferManager)
    {
        for (size_t index = 0; index < std::size(m_UploadPbos); ++index)
        {
            if (m_UploadPbos[index] != 0)
                s_BufferManager->DeleteBuffer(m_UploadPbos[index]);
            m_UploadPbos[index] = 0;
            m_UploadPboCapacities[index] = 0;
        }
    }
    m_UploadPboIndex = 0;

    m_FormatCtx = nullptr;
    m_CodecCtx = nullptr;
    m_SwsCtx = nullptr;
    m_ProceduralFallback = false;
    m_State.store(State::Stopped, std::memory_order_release);
    m_CurrentTime = 0.0;
    m_LastFrameTime = 0.0;
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
    if (m_State.load(std::memory_order_acquire) != State::Playing)
    {
        m_State.store(State::Playing, std::memory_order_release);
        StartDecodeWorker();
        m_DecodeCondition.notify_all();
    }
}

void VideoDecoder::Pause()
{
    m_State.store(State::Paused, std::memory_order_release);
}

void VideoDecoder::Stop()
{
    m_State.store(State::Stopped, std::memory_order_release);
    Seek(0);
}

void VideoDecoder::Update(float dt)
{
    if (m_State.load(std::memory_order_acquire) != State::Playing)
        return;

    // The playback clock, not the render/decode rate, owns presentation timing.
    // Ignore invalid/negative deltas so a bad caller cannot run the media clock
    // backwards or poison it with NaN.
    if (std::isfinite(dt) && dt > 0.0f)
        m_CurrentTime += static_cast<double>(dt) * m_Speed;

    if (m_ProceduralFallback)
    {
        if (m_Loop.load(std::memory_order_acquire) && m_CurrentTime > 8.0)
            m_CurrentTime = 0.0;
        UploadProceduralFrame();
        return;
    }

    const double duration = GetDuration();
    if (m_Loop.load(std::memory_order_acquire) && duration > 0.0 && m_CurrentTime >= duration)
        Seek(std::fmod(m_CurrentTime, duration));

    if (!m_AsyncDecodeEnabled.load(std::memory_order_acquire))
    {
        DecodedFrame decoded;
        int decodedCount = 0;
        bool hasFrame = false;
        while (m_LastFrameTime <= m_CurrentTime && decodedCount < m_MaxDecodeSteps && DecodeFrame(decoded))
        {
            m_UploadPixels = std::move(decoded.pixels);
            decoded = {};
            hasFrame = true;
            ++decodedCount;
        }
        if (hasFrame)
            UploadFrame(m_UploadPixels);
        return;
    }
    bool hasFrame = false;
    bool consumedFrame = false;
    {
        std::lock_guard lock(m_DecodeMutex);
        int consumed = 0;
        while (!m_DecodedFrames.empty() && consumed < m_MaxDecodeSteps)
        {
            // Previously the first queued frame was consumed unconditionally.
            // With a fast render loop that presented one future video frame per
            // engine frame, effectively tying playback speed to FPS. Keep future
            // frames queued until their media timestamp is due.
            if (m_DecodedFrames.front().timestamp > m_CurrentTime)
                break;
            m_UploadPixels = std::move(m_DecodedFrames.front().pixels);
            m_DecodedFrames.pop_front();
            hasFrame = true;
            consumedFrame = true;
            ++consumed;
        }
    }
    if (hasFrame)
        UploadFrame(m_UploadPixels);

    if (consumedFrame)
        m_DecodeCondition.notify_all();
}

bool VideoDecoder::DecodeFrame(DecodedFrame& output)
{
    if (!m_Packet || !m_FormatCtx || !m_CodecCtx || !m_SwsCtx)
        return false;
    bool frameRead = false;

    while (av_read_frame(m_FormatCtx, m_Packet) >= 0)
    {
        if (m_Packet->stream_index == m_VideoStreamIndex)
        {
            if (avcodec_send_packet(m_CodecCtx, m_Packet) == 0)
            {
                while (avcodec_receive_frame(m_CodecCtx, m_Frame) == 0)
                {
                    if (m_Frame->pts != AV_NOPTS_VALUE)
                        m_LastFrameTime = m_Frame->pts * m_TimeBase;
                    else
                        m_LastFrameTime += 1.0 / m_FrameRate;

                    output.timestamp = m_LastFrameTime;
                    output.pixels.resize(static_cast<size_t>(m_OutputWidth) * m_OutputHeight * 4);
                    uint8_t* destination[] = {output.pixels.data(), nullptr, nullptr, nullptr};
                    int destinationLines[] = {m_OutputWidth * 4, 0, 0, 0};
                    sws_scale(m_SwsCtx, (const uint8_t* const*)m_Frame->data, m_Frame->linesize, 0, m_Height,
                              destination, destinationLines);
                    frameRead = true;
                    goto end_decode;
                }
            }
        }
        av_packet_unref(m_Packet);
    }

end_decode:
    av_packet_unref(m_Packet);
    return frameRead;
}

void VideoDecoder::UploadFrame(const std::vector<uint8_t>& pixels)
{
    if (pixels.empty() || !s_TextureManager)
        return;

    auto& tm = GetTextureManager();
    tm.BindTexture(TextureType::Texture2D, m_TextureID);
    if (s_BufferManager)
    {
        const size_t pboIndex = m_UploadPboIndex++ % std::size(m_UploadPbos);
        unsigned int& pbo = m_UploadPbos[pboIndex];
        if (pbo == 0)
            pbo = s_BufferManager->GenBuffer();
        s_BufferManager->BindBuffer(BufferType::PixelUnpackBuffer, pbo);
        if (m_UploadPboCapacities[pboIndex] < pixels.size())
        {
            s_BufferManager->BufferData(BufferType::PixelUnpackBuffer, pixels.size(), nullptr,
                                        BufferUsage::StreamDraw);
            m_UploadPboCapacities[pboIndex] = pixels.size();
        }
        s_BufferManager->BufferSubData(BufferType::PixelUnpackBuffer, 0, pixels.size(), pixels.data());
        tm.TexSubImage2D(TextureType::Texture2D, 0, 0, 0, m_OutputWidth, m_OutputHeight, TextureFormat::RGBA,
                         DataType::UnsignedByte, nullptr);
        s_BufferManager->BindBuffer(BufferType::PixelUnpackBuffer, 0);
    }
    else
    {
        tm.TexSubImage2D(TextureType::Texture2D, 0, 0, 0, m_OutputWidth, m_OutputHeight, TextureFormat::RGBA,
                         DataType::UnsignedByte, pixels.data());
    }
}

void VideoDecoder::StartDecodeWorker()
{
    if (!m_AsyncDecodeEnabled.load(std::memory_order_acquire) || m_ProceduralFallback || !m_FormatCtx ||
        m_DecodeThread.joinable())
        return;
    m_StopDecodeThread.store(false, std::memory_order_release);
    m_DecodeThread = std::thread(&VideoDecoder::DecodeWorkerLoop, this);
}

void VideoDecoder::StopDecodeWorker()
{
    if (!m_DecodeThread.joinable())
        return;
    m_StopDecodeThread.store(true, std::memory_order_release);
    m_DecodeCondition.notify_all();
    m_DecodeThread.join();
    {
        std::lock_guard lock(m_DecodeMutex);
        m_DecodedFrames.clear();
        m_SeekRequested = false;
    }
    m_StopDecodeThread.store(false, std::memory_order_release);
}

void VideoDecoder::DecodeWorkerLoop()
{
    while (!m_StopDecodeThread.load(std::memory_order_acquire))
    {
        std::unique_lock lock(m_DecodeMutex);
        m_DecodeCondition.wait(lock, [this]() {
            return m_StopDecodeThread.load(std::memory_order_acquire) || m_SeekRequested ||
                   (m_State.load(std::memory_order_acquire) == State::Playing &&
                    m_DecodedFrames.size() < MaxQueuedFrames);
        });
        if (m_StopDecodeThread.load(std::memory_order_acquire))
            break;

        if (m_SeekRequested)
        {
            const double target = m_RequestedSeekTime;
            m_SeekRequested = false;
            m_DecodedFrames.clear();
            lock.unlock();
            const int64_t targetPts = static_cast<int64_t>(target / m_TimeBase);
            av_seek_frame(m_FormatCtx, m_VideoStreamIndex, targetPts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(m_CodecCtx);
            m_LastFrameTime = target;
            continue;
        }

        lock.unlock();
        DecodedFrame decoded;
        if (DecodeFrame(decoded))
        {
            std::lock_guard queueLock(m_DecodeMutex);
            if (m_DecodedFrames.size() < MaxQueuedFrames)
                m_DecodedFrames.push_back(std::move(decoded));
            continue;
        }

        if (m_Loop.load(std::memory_order_acquire))
        {
            av_seek_frame(m_FormatCtx, m_VideoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(m_CodecCtx);
            m_LastFrameTime = 0.0;
        }
        else
        {
            m_State.store(State::Stopped, std::memory_order_release);
        }
    }
}

void VideoDecoder::UploadProceduralFrame()
{
    if (!s_TextureManager || m_OutputWidth <= 0 || m_OutputHeight <= 0)
        return;

    m_UploadPixels.resize(static_cast<size_t>(m_OutputWidth) * static_cast<size_t>(m_OutputHeight) * 4);
    auto& pixels = m_UploadPixels;
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
    m_CurrentTime = (std::max)(0.0, timestamp);
    if (!m_AsyncDecodeEnabled.load(std::memory_order_acquire))
    {
        const int64_t targetPts = static_cast<int64_t>(m_CurrentTime / m_TimeBase);
        av_seek_frame(m_FormatCtx, m_VideoStreamIndex, targetPts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(m_CodecCtx);
        m_LastFrameTime = m_CurrentTime;
        return;
    }
    {
        std::lock_guard lock(m_DecodeMutex);
        m_RequestedSeekTime = m_CurrentTime;
        m_SeekRequested = true;
        m_DecodedFrames.clear();
    }
    if (m_State.load(std::memory_order_acquire) != State::Stopped)
        StartDecodeWorker();
    m_DecodeCondition.notify_all();
}
