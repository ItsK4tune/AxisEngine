#include <ecs/systems/audio_system.h>
#include <ecs/entity_manager.h>
#include <ecs/components/camera_component.h>
#include <ecs/components/transform_component.h>
#include <audio/sound_player.h>
#include <utils/logger.h>

void AudioSystem::Update(Scene &scene, SoundPlayer &soundPlayer)
{
    if (!m_Enabled)
        return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity != entt::null)
    {
        auto &cam = scene.registry.get<CameraComponent>(camEntity);
        auto &camTrans = scene.registry.get<TransformComponent>(camEntity);

        glm::vec3 lookDir = glm::vec3(0.0f, 0.0f, -1.0f);
        if (glm::length(cam.front) > 0.1f) {
            lookDir = cam.front;
        }

        soundPlayer.UpdateListener(camTrans.position, lookDir);
    }

    auto view = scene.registry.view<AudioSourceComponent>();

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

            if (audio.is3D)
            {
                TransformComponent *transform = scene.registry.try_get<TransformComponent>(entity);
                glm::vec3 pos = transform ? transform->position : glm::vec3(0.0f);

                auto source = soundPlayer.GetEngine()->AddSoundSourceFromFile(audio.filePath);

                if (source)
                {
                    source->SetDefaultMinDistance(audio.minDistance);

                    audio.sound = soundPlayer.Play3D(audio.filePath, pos, audio.loop);
                }
            }
            else
            {
                audio.sound = soundPlayer.GetEngine()->Play2D(audio.filePath, audio.loop, false );
            }

            if (audio.sound)
            {
                audio.sound->SetVolume(audio.volume);
            }
        }

        if (audio.is3D && audio.sound && !audio.sound->IsFinished())
        {
            TransformComponent *transform = scene.registry.try_get<TransformComponent>(entity);
            if (transform)
            {
                audio.sound->SetPosition(transform->position);
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
