#include <ecs/system.h>
#include <graphic/core/video_decoder.h>
#include <utils/logger.h>
#include <iostream>
#include <string>

void VideoSystem::Update(Scene &scene, ResourceManager &res, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<VideoPlayerComponent>();

    for (auto entity : view)
    {
        auto &video = view.get<VideoPlayerComponent>(entity);

        if (!video.isLoaded)
        {
            if (!video.decoder)
            {
                video.decoder = new VideoDecoder();
            }

            if (video.decoder->Load(video.filePath))
            {
                video.isLoaded = true;
                video.decoder->SetLoop(video.isLooping);
                video.decoder->SetSpeed(video.speed);
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);

                if (scene.registry.all_of<UITransformComponent>(entity))
                {
                    auto &ui = scene.registry.get<UITransformComponent>(entity);
                    if (ui.size.x > 0 && ui.size.y > 0)
                    {
                        video.decoder->SetOutputSize((int)ui.size.x, (int)ui.size.y);
                        LOGGER_INFO("VideoSystem") << "Auto-scaling video to UI size: " << ui.size.x << "x" << ui.size.y;
                    }
                }

                if (video.playOnAwake)
                {
                    video.decoder->Play();
                    video.isPlaying = true;
                }
            }
            else
            {
                LOGGER_ERROR("VideoSystem") << "Failed to load: " << video.filePath;
                video.isLoaded = true;
                delete video.decoder;
                video.decoder = nullptr;
                continue;
            }
        }

        if (video.decoder && video.isPlaying)
        {
            if (video.decoder->IsLooping() != video.isLooping)
                video.decoder->SetLoop(video.isLooping);

            if (video.decoder->GetSpeed() != video.speed)
                video.decoder->SetSpeed(video.speed);

            if (video.decoder->GetMaxDecodeSteps() != video.maxDecodes)
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);

            video.decoder->Update(dt);

            if (auto *uiRenderer = scene.registry.try_get<UIRendererComponent>(entity))
            {
                std::string uniqueName = "video_ui_" + std::to_string((uint32_t)entity);

                if (!res.GetUIModel(uniqueName))
                {
                    res.CreateUIModel(uniqueName, UIType::Texture);
                    uiRenderer->model = res.GetUIModel(uniqueName);
                }

                if (uiRenderer->model && uiRenderer->model != res.GetUIModel(uniqueName))
                {
                    uiRenderer->model = res.GetUIModel(uniqueName);
                }

                if (uiRenderer->model)
                {
                    uiRenderer->model->SetTexture(video.decoder->GetTextureID());
                }
            }

            if (auto *renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            {
                if (renderer->model)
                {
                    for (auto &mesh : renderer->model->meshes)
                    {
                        Texture videoTex;
                        videoTex.id = video.decoder->GetTextureID();
                        videoTex.type = "texture_diffuse";
                        videoTex.path = "video_stream";

                        mesh.textures.clear();
                        mesh.textures.push_back(videoTex);
                    }
                }
            }
        }
    }
}
