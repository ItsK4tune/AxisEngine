#include <ecs/system.h>
#include <ecs/component.h>

#include <audio/sound_player.h>
#include <utils/logger.h>

void AudioSystem::Update(Scene &scene, SoundPlayer &soundPlayer)
{
    if (!m_Enabled)
        return;

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
