#include <ecs/logic/audio_system.h>
#include <audio/interface/i_audio_capture_service.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <algorithm>


void AudioSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<AudioSystem>(this);
    m_EventSubscriptions.Clear();
    m_AppliedSources.clear();
    m_AppliedGlobalVolume = std::numeric_limits<float>::quiet_NaN();
    if (m_BoundScene)
    {
        m_BoundScene->GetRegistry().on_destroy<AudioSourceComponent>().disconnect<&AudioSystem::OnAudioSourceDestroyed>(
            this);
        m_BoundScene = nullptr;
    }

    auto* configManager = sl.Resolve<ConfigManager>();
    if (configManager)
    {
        m_GlobalVolume = configManager->GetConfigSnapshot()->audio.masterVolume;

        m_EventSubscriptions.Add(
            EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
                if (HasConfigChanged(e, ConfigChangedEvent::Audio))
                {
                    m_GlobalVolume = e.config.audio.masterVolume;
                }
            }));
    }

    auto* scene = sl.Resolve<Scene>();
    if (scene)
    {
        scene->GetRegistry().on_destroy<AudioSourceComponent>().connect<&AudioSystem::OnAudioSourceDestroyed>(this);
        m_BoundScene = scene;
    }
}

void AudioSystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    if (m_BoundScene)
    {
        m_BoundScene->GetRegistry().on_destroy<AudioSourceComponent>().disconnect<&AudioSystem::OnAudioSourceDestroyed>(
            this);
        m_BoundScene = nullptr;
    }
    m_AppliedSources.clear();
}

void AudioSystem::Update(Scene& scene, float dt)
{
    auto* audioService = ServiceLocator::Instance().Resolve<AudioService>();
    if (!audioService)
        return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto& cam = scene.GetRegistry().get<CameraComponent>(camEntity);
        auto& camPos = scene.GetRegistry().get<PositionComponent>(camEntity);
        auto& camRot = scene.GetRegistry().get<RotationComponent>(camEntity);

        glm::vec3 listenerPosition = camPos.value;
        glm::vec3 lookDir = camRot.value * glm::vec3(0.0f, 0.0f, -1.0f);
        if (auto* world = scene.TryGetComponent<WorldTransformComponent>(camEntity))
        {
            listenerPosition = glm::vec3(world->worldMatrix[3]);
            lookDir = glm::normalize(-glm::vec3(world->worldMatrix[2]));
        }

        audioService->UpdateListener(listenerPosition, lookDir);
        if (auto* capture = ServiceLocator::Instance().Resolve<IAudioCaptureService>())
            capture->SetPulseOrigin(listenerPosition);
    }

    auto* engine = audioService->GetEngine();
    if (!engine)
        return;

    auto view = scene.GetRegistry().view<AudioSourceComponent, InfoComponent>();

    if (m_AppliedGlobalVolume != m_GlobalVolume)
    {
        engine->SetGlobalVolume(m_GlobalVolume);
        m_AppliedGlobalVolume = m_GlobalVolume;
    }

    for (auto entity : view)
    {
        auto& audio = view.get<AudioSourceComponent>(entity);
        auto& info = view.get<InfoComponent>(entity);

        if (!info.isActive)
        {
            if (audio.sound)
            {
                audio.sound->Stop();
                audio.sound = nullptr;
                m_AppliedSources.erase(entity);
            }
            continue;
        }

        if (audio.playOnAwake && !audio.sound && !audio.shouldPlay)
        {
            audio.shouldPlay = true;
            audio.playOnAwake = false;
        }

        if (audio.shouldPlay)
        {
            audio.shouldPlay = false;

            if (audio.sound)
            {
                audio.sound->Stop();
                audio.sound = nullptr;
                m_AppliedSources.erase(entity);
            }

            glm::vec3 sourcePosition(0.0f);
            if (auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
                sourcePosition = glm::vec3(world->worldMatrix[3]);
            else if (auto* posComp = scene.TryGetComponent<PositionComponent>(entity))
                sourcePosition = posComp->value;

            if (audio.source)
            {
                if (audio.is3D)
                {
                    audio.sound = engine->Play3D(audio.source.get(), sourcePosition, audio.loop);
                }
                else
                {
                    audio.sound = engine->Play2D(audio.source.get(), audio.loop);
                }
            }
            else if (!audio.filePath.empty())
            {
                const std::string resolvedPath = FileSystem::getPath(audio.filePath);
                if (audio.is3D)
                {
                    audio.sound = engine->Play3D(resolvedPath, sourcePosition, audio.loop);
                }
                else
                {
                    audio.sound = engine->Play2D(resolvedPath, audio.loop);
                }
            }

            if (audio.sound)
            {
                m_AppliedSources.erase(entity);
            }
        }

        if (audio.sound)
        {
            const bool finished = audio.sound->IsFinished();
            if (finished)
            {
                audio.sound = nullptr;
                m_AppliedSources.erase(entity);
                continue;
            }

            auto& applied = m_AppliedSources[entity];
            if (applied.sound != audio.sound.get())
            {
                applied = {};
                applied.sound = audio.sound.get();
            }
            if (applied.volume != audio.volume)
            {
                audio.sound->SetVolume(audio.volume);
                applied.volume = audio.volume;
            }
            const float effectivePitch = audio.pitch * audio.speed;
            if (applied.pitch != effectivePitch)
            {
                audio.sound->SetPitch(effectivePitch);
                applied.pitch = effectivePitch;
            }

            if (audio.is3D)
            {
                audio.minDistance = (std::max)(0.001f, audio.minDistance);
                audio.maxDistance = (std::max)(audio.minDistance, audio.maxDistance);
                glm::vec3 sourcePosition(0.0f);
                uint32_t transformVersion = 0;
                if (auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
                {
                    sourcePosition = glm::vec3(world->worldMatrix[3]);
                    transformVersion = world->version;
                }
                else if (auto* posComp = scene.TryGetComponent<PositionComponent>(entity))
                    sourcePosition = posComp->value;
                if (applied.transformVersion != transformVersion ||
                    glm::any(glm::notEqual(applied.position, sourcePosition)))
                {
                    audio.sound->SetPosition(sourcePosition);
                    applied.position = sourcePosition;
                    applied.transformVersion = transformVersion;
                }
                if (glm::any(glm::notEqual(applied.velocity, audio.velocity)))
                {
                    audio.sound->SetVelocity(audio.velocity);
                    applied.velocity = audio.velocity;
                }
                if (applied.minDistance != audio.minDistance)
                {
                    audio.sound->SetMinDistance(audio.minDistance);
                    applied.minDistance = audio.minDistance;
                }
                if (applied.maxDistance != audio.maxDistance)
                {
                    audio.sound->SetMaxDistance(audio.maxDistance);
                    applied.maxDistance = audio.maxDistance;
                }
            }
            else if (applied.pan != audio.pan)
            {
                audio.sound->SetPan(audio.pan);
                applied.pan = audio.pan;
            }
        }
    }

    engine->Update();
}

void AudioSystem::OnAudioSourceDestroyed(entt::registry& registry, entt::entity entity)
{
    auto& audio = registry.get<AudioSourceComponent>(entity);
    if (audio.sound)
    {
        audio.sound->Stop();
        audio.sound = nullptr;
    }
    m_AppliedSources.erase(entity);
}

void AudioSystem::StopAll(Scene& scene)
{
    auto view = scene.GetRegistry().view<AudioSourceComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioSourceComponent>(entity);
        if (audio.sound)
        {
            audio.sound->Stop();
            audio.sound = nullptr;
            m_AppliedSources.erase(entity);
        }
    }
}

std::vector<entt::id_type> AudioSystem::GetReadComponents() const
{
    return {entt::type_id<CameraComponent>().hash(), entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash()};
}

std::vector<entt::id_type> AudioSystem::GetWriteComponents() const
{
    return {entt::type_id<AudioSourceComponent>().hash()};
}

void AudioSystem::SetEnabled(bool enable)
{
    m_Enabled = enable;
    auto* audioService = ServiceLocator::Instance().Resolve<AudioService>();
    if (audioService && audioService->GetEngine())
    {
        if (!enable)
        {
            audioService->GetEngine()->SetGlobalVolume(0.0f);
            m_AppliedGlobalVolume = 0.0f;
        }
        else
        {
            audioService->GetEngine()->SetGlobalVolume(m_GlobalVolume);
            m_AppliedGlobalVolume = m_GlobalVolume;
        }
    }
}
