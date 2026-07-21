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
#include <resource/logic/resource_manager.h>
#include <iostream>
#include <memory>
#include <string>


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
                    video.isPlaying = true;
                }
            }
            else
            {
                LOGGER_ERROR("VideoSystem") << "Failed to load: " << video.filePath;
                video.isLoaded = true;
                video.decoder.reset();
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
    if (!m_BoundScene)
        return;
    auto view = m_BoundScene->View<VideoPlayerComponent>();
    for (auto entity : view)
    {
        if (auto& video = view.get<VideoPlayerComponent>(entity); video.decoder)
            video.decoder->SetAsyncDecodeEnabled(m_AsyncDecodeEnabled);
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
