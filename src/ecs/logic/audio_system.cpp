#include <ecs/logic/audio_system.h>
#include <ecs/logic/system_factory.h>
#include <audio/logic/audio_service.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/core_components.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/type/app_config.h>

REGISTER_SYSTEM(AudioSystem)

void AudioSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<AudioSystem>(this);
    auto& configManager = sl.Require<ConfigManager>();
    m_GlobalVolume = configManager.GetConfig().masterVolume;

    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (e.bitmask & (ConfigChangedEvent::Audio | ConfigChangedEvent::All)) {
            m_GlobalVolume = e.config.masterVolume;
        }
    });
}

void AudioSystem::Update(Scene &scene, float dt)
{
    auto* audioService = ServiceLocator::Instance().Resolve<AudioService>();
    if (!audioService)
        return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity != entt::null)
    {
        auto &cam = scene.registry.get<CameraComponent>(camEntity);
        auto &camPos = scene.registry.get<PositionComponent>(camEntity);

        glm::vec3 lookDir = glm::vec3(0.0f, 0.0f, -1.0f);
        if (glm::length(cam.front) > 0.1f) {
            lookDir = cam.front;
        }

        audioService->UpdateListener(camPos.value, lookDir);
    }

    auto view = scene.registry.view<AudioSourceComponent>();


    audioService->GetEngine()->SetGlobalVolume(m_GlobalVolume);

    for (auto entity : view)
    {
        auto &audio = view.get<AudioSourceComponent>(entity);

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
            }

            if (audio.source)
            {
                if (audio.is3D)
                {
                    PositionComponent *posComp = scene.registry.try_get<PositionComponent>(entity);
                    glm::vec3 pos = posComp ? posComp->value : glm::vec3(0.0f);
                    audio.sound = audioService->GetEngine()->Play3D(audio.source.get(), pos, audio.loop);
                }
                else
                {
                    audio.sound = audioService->GetEngine()->Play2D(audio.source.get(), audio.loop);
                }
            }
            else if (!audio.filePath.empty())
            {

                if (audio.is3D)
                {
                    PositionComponent *posComp = scene.registry.try_get<PositionComponent>(entity);
                    glm::vec3 pos = posComp ? posComp->value : glm::vec3(0.0f);
                    audio.sound = audioService->GetEngine()->Play3D(audio.filePath, pos, audio.loop);
                }
                else
                {
                    audio.sound = audioService->GetEngine()->Play2D(audio.filePath, audio.loop);
                }
            }

            if (audio.sound)
            {

                audio.sound->SetVolume(audio.volume);
                audio.sound->SetPitch(audio.pitch * audio.speed);
                audio.sound->SetPan(audio.pan);
                if (audio.is3D) {
                    audio.sound->SetMinDistance(audio.minDistance);
                    audio.sound->SetMaxDistance(audio.maxDistance);
                    audio.sound->SetVelocity(audio.velocity);
                }
            }
        }

        if (audio.sound && !audio.sound->IsFinished())
        {

            audio.sound->SetVolume(audio.volume);
            audio.sound->SetPitch(audio.pitch * audio.speed);
            audio.sound->SetPan(audio.pan);

            if (audio.is3D)
            {
                PositionComponent *posComp = scene.registry.try_get<PositionComponent>(entity);
                if (posComp)
                {
                    audio.sound->SetPosition(posComp->value);
                }
                audio.sound->SetVelocity(audio.velocity);
                audio.sound->SetMinDistance(audio.minDistance);
                audio.sound->SetMaxDistance(audio.maxDistance);
            }
        }

        if (audio.sound && audio.sound->IsFinished())
        {
            audio.sound = nullptr;
        }
    }
}

void AudioSystem::StopAll(Scene &scene)
{
    auto view = scene.registry.view<AudioSourceComponent>();
    for (auto entity : view)
    {
        auto &audio = view.get<AudioSourceComponent>(entity);
        if (audio.sound)
        {
            audio.sound->Stop();
            audio.sound = nullptr;
        }
    }
}

std::vector<entt::id_type> AudioSystem::GetReadComponents() const
{
    return {entt::type_id<CameraComponent>().hash(), entt::type_id<PositionComponent>().hash()};
}

std::vector<entt::id_type> AudioSystem::GetWriteComponents() const
{
    return {entt::type_id<AudioSourceComponent>().hash()};
}
