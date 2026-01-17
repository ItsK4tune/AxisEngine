#include <ecs/system.h>
#include <graphic/video_decoder.h>
#include <iostream>
#include <string>

void VideoSystem::Update(Scene &scene, ResourceManager &res, float dt)
{
    if (!m_Enabled) return;

    auto view = scene.registry.view<VideoPlayerComponent>();

    for (auto entity : view)
    {
        auto &video = view.get<VideoPlayerComponent>(entity);

        // 1. Initialize Decoder
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
                if (video.playOnAwake)
                {
                    video.decoder->Play();
                    video.isPlaying = true;
                }
            }
            else
            {
                std::cerr << "[VideoSystem] Failed to load: " << video.filePath << std::endl;
                video.isLoaded = true; // Mark as done to avoid loop
                delete video.decoder;
                video.decoder = nullptr;
                continue;
            }
        }

        // 2. Update Decoder
        if (video.decoder && video.isPlaying)
        {
            if (video.decoder->IsLooping() != video.isLooping)
                video.decoder->SetLoop(video.isLooping);
            
            if (video.decoder->GetSpeed() != video.speed)
                video.decoder->SetSpeed(video.speed);

            video.decoder->Update(dt);

            // 3. Update Sync Component (UI Renderer)
            if (auto *uiRenderer = scene.registry.try_get<UIRendererComponent>(entity))
            {
                // Ensure we have a dedicated model for this video
                std::string uniqueName = "video_ui_" + std::to_string((uint32_t)entity);
                
                if (!res.GetUIModel(uniqueName))
                {
                     res.CreateUIModel(uniqueName, UIType::Texture);
                     uiRenderer->model = res.GetUIModel(uniqueName);
                }
                
                // If model is still pointing to shared default, switch it
                if (uiRenderer->model && uiRenderer->model != res.GetUIModel(uniqueName))
                {
                    uiRenderer->model = res.GetUIModel(uniqueName);
                }

                if (uiRenderer->model)
                {
                    uiRenderer->model->SetTexture(video.decoder->GetTextureID());
                }
            }
        }
    }
}
