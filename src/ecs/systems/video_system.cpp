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
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);

                // Auto-scale optimization for UI
                if (scene.registry.all_of<UITransformComponent>(entity)) {
                    auto& ui = scene.registry.get<UITransformComponent>(entity);
                    if (ui.size.x > 0 && ui.size.y > 0) {
                        video.decoder->SetOutputSize((int)ui.size.x, (int)ui.size.y);
                        // std::cout << "[VideoSystem] Auto-scaling video to UI size: " << ui.size.x << "x" << ui.size.y << std::endl;
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

            if (video.decoder->GetMaxDecodeSteps() != video.maxDecodes)
                video.decoder->SetMaxDecodeSteps(video.maxDecodes);

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

            // 4. Update Mesh Renderer (3D Model)
            if (auto *renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            {
                if (renderer->model)
                {
                    // For every mesh in the model, override the texture
                    // Note: This modifies the SHARED resource model. All entities using this model will show the video.
                    // To avoid this, we would need to clone the model resource.
                    for (auto &mesh : renderer->model->meshes)
                    {
                        // Create a texture struct wrapper
                        Texture videoTex;
                        videoTex.id = video.decoder->GetTextureID();
                        videoTex.type = "texture_diffuse"; // Standard shader uniform name
                        videoTex.path = "video_stream";

                        // Replace existing textures or add if empty
                        // Ideally we clear and set only this one to avoid conflicts
                        mesh.textures.clear();
                        mesh.textures.push_back(videoTex);
                    }
                }
            }
        }
    }
}
