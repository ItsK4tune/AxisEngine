#include <graphic/video_decoder.h>
#include <iostream>
#include <glad/glad.h>

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

bool VideoDecoder::Load(const std::string &filepath)
{
    Unload();
    m_Filepath = filepath;

    av_log_set_level(AV_LOG_QUIET);
    if (avformat_open_input(&m_FormatCtx, filepath.c_str(), nullptr, nullptr) != 0)
    {
        std::cerr << "Failed to open video file: " << filepath << std::endl;
        return false;
    }

    if (avformat_find_stream_info(m_FormatCtx, nullptr) < 0)
    {
        std::cerr << "Failed to find stream info" << std::endl;
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
        std::cerr << "No video stream found" << std::endl;
        return false;
    }

    AVCodecParameters *codecPar = m_FormatCtx->streams[m_VideoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        std::cerr << "Unsupported codec" << std::endl;
        return false;
    }

    m_CodecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_CodecCtx, codecPar);

    if (avcodec_open2(m_CodecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec" << std::endl;
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

    if (!m_CodecCtx)
        return;

    if (m_SwsCtx)
        sws_freeContext(m_SwsCtx);
    m_SwsCtx = sws_getContext(
        m_Width, m_Height, m_CodecCtx->pix_fmt,
        m_OutputWidth, m_OutputHeight, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (m_RGBFrame->data[0])
        av_freep(&m_RGBFrame->data[0]);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_OutputWidth, m_OutputHeight, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(m_RGBFrame->data, m_RGBFrame->linesize, buffer, AV_PIX_FMT_RGBA, m_OutputWidth, m_OutputHeight, 1);

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

    if (m_TextureID != 0)
    {
        glDeleteTextures(1, &m_TextureID);
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
    m_State = State::Stopped;
}

void VideoDecoder::InitTexture()
{
    if (m_TextureID == 0)
        glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_OutputWidth, m_OutputHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
    AVPacket *packet = av_packet_alloc();
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
    if (!m_SwsCtx || !m_Frame || !m_RGBFrame)
        return;

    sws_scale(m_SwsCtx,
              (const uint8_t *const *)m_Frame->data, m_Frame->linesize,
              0, m_Height,
              m_RGBFrame->data, m_RGBFrame->linesize);

    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_OutputWidth, m_OutputHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_RGBFrame->data[0]);
}

void VideoDecoder::Seek(double timestamp)
{
    if (!m_FormatCtx)
        return;
    int64_t targetPts = (int64_t)(timestamp / m_TimeBase);
    av_seek_frame(m_FormatCtx, m_VideoStreamIndex, targetPts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_CodecCtx);
}
