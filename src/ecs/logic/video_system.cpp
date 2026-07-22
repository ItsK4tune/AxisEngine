#include <ecs/logic/video_system.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <audio/interface/i_audio_engine.h>
#include <resource/logic/resource_manager.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>
#include <optional>
#include <future>
#include <chrono>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

namespace
{
void WriteLE16(std::ostream& stream, uint16_t value)
{
    const char bytes[] = {static_cast<char>(value), static_cast<char>(value >> 8)};
    stream.write(bytes, sizeof(bytes));
}

void WriteLE32(std::ostream& stream, uint32_t value)
{
    const char bytes[] = {static_cast<char>(value), static_cast<char>(value >> 8), static_cast<char>(value >> 16),
                          static_cast<char>(value >> 24)};
    stream.write(bytes, sizeof(bytes));
}

std::optional<std::string> DecodeEmbeddedAudioToWav(const std::string& sourcePath)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto sourceSize = fs::file_size(sourcePath, ec);
    if (ec)
        return std::nullopt;
    const auto sourceTime = fs::last_write_time(sourcePath, ec);
    if (ec)
        return std::nullopt;
    const size_t cacheKey = std::hash<std::string>{}(sourcePath + std::to_string(sourceSize) +
                                                     std::to_string(sourceTime.time_since_epoch().count()));
    const fs::path cacheDir = fs::temp_directory_path(ec) / "axisengine_video_audio";
    if (ec)
        return std::nullopt;
    fs::create_directories(cacheDir, ec);
    if (ec)
        return std::nullopt;
    const fs::path outputPath = cacheDir / (std::to_string(cacheKey) + ".wav");
    if (fs::exists(outputPath, ec) && fs::file_size(outputPath, ec) > 44)
        return outputPath.string();

    AVFormatContext* format = nullptr;
    AVCodecContext* decoder = nullptr;
    SwrContext* resampler = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    AVChannelLayout outputLayout{};
    auto cleanup = [&]() {
        av_frame_free(&frame);
        av_packet_free(&packet);
        swr_free(&resampler);
        avcodec_free_context(&decoder);
        avformat_close_input(&format);
        av_channel_layout_uninit(&outputLayout);
    };

    if (avformat_open_input(&format, sourcePath.c_str(), nullptr, nullptr) < 0 ||
        avformat_find_stream_info(format, nullptr) < 0)
    {
        cleanup();
        return std::nullopt;
    }
    const int audioStream = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStream < 0)
    {
        cleanup();
        return std::nullopt;
    }
    const AVCodecParameters* parameters = format->streams[audioStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
    decoder = codec ? avcodec_alloc_context3(codec) : nullptr;
    if (!decoder || avcodec_parameters_to_context(decoder, parameters) < 0 || avcodec_open2(decoder, codec, nullptr) < 0)
    {
        cleanup();
        return std::nullopt;
    }

    constexpr int OutputRate = 48000;
    constexpr int OutputChannels = 2;
    constexpr int BytesPerSample = 2;
    av_channel_layout_default(&outputLayout, OutputChannels);
    if (decoder->ch_layout.nb_channels == 0)
        av_channel_layout_copy(&decoder->ch_layout, &parameters->ch_layout);
    if (swr_alloc_set_opts2(&resampler, &outputLayout, AV_SAMPLE_FMT_S16, OutputRate, &decoder->ch_layout,
                            decoder->sample_fmt, decoder->sample_rate, 0, nullptr) < 0 ||
        !resampler || swr_init(resampler) < 0)
    {
        cleanup();
        return std::nullopt;
    }

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        cleanup();
        return std::nullopt;
    }
    output.write("RIFF", 4); WriteLE32(output, 0); output.write("WAVEfmt ", 8); WriteLE32(output, 16);
    WriteLE16(output, 1); WriteLE16(output, OutputChannels); WriteLE32(output, OutputRate);
    WriteLE32(output, OutputRate * OutputChannels * BytesPerSample);
    WriteLE16(output, OutputChannels * BytesPerSample); WriteLE16(output, BytesPerSample * 8);
    output.write("data", 4); WriteLE32(output, 0);

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    uint64_t dataBytes = 0;
    bool decodeOk = packet && frame;
    auto drainFrames = [&]() {
        while (decodeOk && avcodec_receive_frame(decoder, frame) == 0)
        {
            const int maxSamples = swr_get_out_samples(resampler, frame->nb_samples);
            std::vector<uint8_t> pcm(static_cast<size_t>((std::max)(0, maxSamples)) * OutputChannels * BytesPerSample);
            uint8_t* destination = pcm.data();
            const int converted = swr_convert(resampler, &destination, maxSamples,
                                              const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);
            if (converted < 0)
            {
                decodeOk = false;
                break;
            }
            const size_t bytes = static_cast<size_t>(converted) * OutputChannels * BytesPerSample;
            if (dataBytes + bytes > UINT32_MAX)
            {
                decodeOk = false;
                break;
            }
            output.write(reinterpret_cast<const char*>(pcm.data()), static_cast<std::streamsize>(bytes));
            dataBytes += bytes;
            av_frame_unref(frame);
        }
    };
    while (decodeOk && av_read_frame(format, packet) >= 0)
    {
        if (packet->stream_index == audioStream && avcodec_send_packet(decoder, packet) >= 0)
            drainFrames();
        av_packet_unref(packet);
    }
    if (decodeOk && avcodec_send_packet(decoder, nullptr) >= 0)
        drainFrames();

    output.seekp(4); WriteLE32(output, static_cast<uint32_t>(36 + dataBytes));
    output.seekp(40); WriteLE32(output, static_cast<uint32_t>(dataBytes));
    output.close();
    cleanup();
    if (!decodeOk || dataBytes == 0)
    {
        fs::remove(outputPath, ec);
        return std::nullopt;
    }
    return outputPath.string();
}
}


void VideoSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<VideoSystem>(this);
    if (m_BoundScene)
    {
        m_BoundScene->GetRegistry().on_destroy<VideoPlayerComponent>().disconnect<&VideoSystem::OnVideoPlayerDestroyed>(
            this);
        m_BoundScene = nullptr;
    }

    auto* scene = sl.Resolve<Scene>();
    if (scene)
    {
        scene->GetRegistry().on_destroy<VideoPlayerComponent>().connect<&VideoSystem::OnVideoPlayerDestroyed>(this);
        m_BoundScene = scene;
    }
}

void VideoSystem::Shutdown()
{
    if (m_BoundScene)
    {
        m_BoundScene->GetRegistry().on_destroy<VideoPlayerComponent>().disconnect<&VideoSystem::OnVideoPlayerDestroyed>(
            this);
        m_BoundScene = nullptr;
    }
}

void VideoSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.GetRegistry().view<VideoPlayerComponent>();
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();

    for (auto entity : view)
    {
        auto& video = view.get<VideoPlayerComponent>(entity);
        if (!video.isLoaded)
        {
            video.loadRetryRemaining = (std::max)(0.0f, video.loadRetryRemaining - (std::max)(0.0f, dt));
            if (video.loadRetryRemaining > 0.0f)
                continue;
            if (!video.decoder)
            {
                video.decoder = std::make_shared<VideoDecoder>();
            }

            if (video.decoder->Load(FileSystem::getPath(video.filePath)))
            {
                video.isLoaded = true;
                video.decoder->SetLoop(video.isLooping);
                video.decoder->SetSpeed(video.speed);
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);
                video.decoder->SetAsyncDecodeEnabled(m_AsyncDecodeEnabled);
                video.decoder->SetMaxQueuedFrames(m_DecodeQueueSize);

                if (video.decoder->HasAudioStream())
                {
                    const std::string resolvedPath = FileSystem::getPath(video.filePath);
                    video.embeddedAudioFuture = std::async(std::launch::async, [resolvedPath]() {
                        return DecodeEmbeddedAudioToWav(resolvedPath);
                    }).share();
                    video.embeddedAudioPending = true;
                }

                if (scene.GetRegistry().all_of<UITransformComponent>(entity))
                {
                    auto& ui = scene.GetRegistry().get<UITransformComponent>(entity);
                    if (ui.size.x > 0 && ui.size.y > 0)
                    {
                        video.decoder->SetOutputSize((int)ui.size.x, (int)ui.size.y);
                        LOGGER_INFO("VideoSystem")
                            << "Auto-scaling video to UI size: " << ui.size.x << "x" << ui.size.y;
                    }
                }

                if (video.playOnAwake)
                {
                    video.decoder->Play();
                    if (video.audio)
                        video.audio->Resume();
                    video.isPlaying = true;
                }
            }
            else
            {
                LOGGER_ERROR("VideoSystem") << "Failed to load: " << video.filePath;
                video.decoder.reset();
                video.loadRetryRemaining = m_LoadRetrySeconds;
                continue;
            }
        }

        if (video.decoder && video.isPlaying)
        {
            if (video.embeddedAudioPending && video.embeddedAudioFuture.valid() &&
                video.embeddedAudioFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                video.embeddedAudioPending = false;
                const auto decodedAudio = video.embeddedAudioFuture.get();
                if (decodedAudio)
                {
                    if (auto* audioEngine = ServiceLocator::Instance().Resolve<IAudioEngine>())
                    {
                        video.audio = audioEngine->Play2D(*decodedAudio, video.isLooping, true);
                        if (video.audio)
                        {
                            video.audio->SetVolume((std::clamp)(video.volume, 0.0f, 1.0f) * 100.0f);
                            video.audio->SetPitch((std::max)(0.1f, video.speed));
                            video.audio->SetPlayPosition(static_cast<unsigned int>(
                                (std::max)(0.0, video.decoder->GetCurrentTime()) * 1000.0));
                            video.audio->Resume();
                        }
                    }
                }
                else
                    LOGGER_WARN("VideoSystem") << "Could not decode embedded audio from " << video.filePath;
            }
            if (video.decoder->IsLooping() != video.isLooping)
            {
                video.decoder->SetLoop(video.isLooping);
                if (video.audio)
                    video.audio->SetIsLooped(video.isLooping);
            }

            if (video.decoder->GetSpeed() != video.speed)
            {
                video.decoder->SetSpeed(video.speed);
                if (video.audio)
                    video.audio->SetPitch((std::max)(0.1f, video.speed));
            }

            if (video.audio)
            {
                video.audio->SetVolume((std::clamp)(video.volume, 0.0f, 1.0f) * 100.0f);
                const double audioTime = static_cast<double>(video.audio->GetPlayPosition()) / 1000.0;
                const double videoTime = video.decoder->GetCurrentTime();
                if (std::abs(audioTime - videoTime) > m_AVSyncThresholdSeconds)
                    video.audio->SetPlayPosition(static_cast<unsigned int>((std::max)(0.0, videoTime) * 1000.0));
            }

            if (video.decoder->GetMaxDecodeSteps() != video.maxDecodes)
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);

            video.decoder->Update(dt);
            if (!video.decoder->IsPlaying())
            {
                video.isPlaying = false;
                if (video.audio)
                    video.audio->Pause();
            }

            if (auto* uiRenderer = scene.GetRegistry().try_get<UIRendererComponent>(entity))
            {
                if (!uiRenderer->model)
                {
                    std::string uniqueName = "video_ui_" + std::to_string((uint32_t)entity);

                    if (!resources.GetUIModel(uniqueName))
                    {
                        resources.CreateUIModel(uniqueName, ::UIType::Texture);
                    }
                    uiRenderer->model = resources.GetUIModel(uniqueName);
                }

                if (uiRenderer->model)
                {
                    uiRenderer->model->SetTexture(video.decoder->GetTextureID());
                }
            }

            if (scene.GetRegistry().all_of<MeshRendererComponent>(entity))
            {
                auto& mat = scene.GetOrAddComponent<MaterialComponent>(entity);
                mat.gpu.albedoMap = video.decoder->GetTextureID();
                mat.gpu.dirty = false;
            }
        }
    }
}

void VideoSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    m_AsyncDecodeEnabled = config.videoAsyncDecodeEnabled;
    m_DecodeQueueSize = static_cast<size_t>((std::max)(1, config.videoDecodeQueueSize));
    m_AVSyncThresholdSeconds = (std::max)(0.0f, config.videoAVSyncThresholdSeconds);
    m_LoadRetrySeconds = (std::max)(0.05f, config.videoLoadRetrySeconds);
    if (!m_BoundScene)
        return;
    auto view = m_BoundScene->View<VideoPlayerComponent>();
    for (auto entity : view)
    {
        if (auto& video = view.get<VideoPlayerComponent>(entity); video.decoder)
        {
            video.decoder->SetAsyncDecodeEnabled(m_AsyncDecodeEnabled);
            video.decoder->SetMaxQueuedFrames(m_DecodeQueueSize);
        }
    }
}

void VideoSystem::OnVideoPlayerDestroyed(entt::registry& registry, entt::entity entity)
{
    auto& video = registry.get<VideoPlayerComponent>(entity);
    if (video.decoder)
    {
        video.decoder->Stop();
        video.decoder.reset();
    }
    if (video.audio)
    {
        video.audio->Stop();
        video.audio.reset();
    }
}

std::vector<entt::id_type> VideoSystem::GetReadComponents() const
{
    return {entt::type_id<UITransformComponent>().hash()};
}

std::vector<entt::id_type> VideoSystem::GetWriteComponents() const
{
    return {entt::type_id<VideoPlayerComponent>().hash(), entt::type_id<UIRendererComponent>().hash(),
            entt::type_id<MeshRendererComponent>().hash()};
}
