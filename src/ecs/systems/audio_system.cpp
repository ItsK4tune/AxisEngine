#include <ecs/system.h>
#include <ecs/component.h>

#include <audio/sound_player.h>

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
                // Smart pointers handle cleanup, but we might want to forcefully stop.
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
                    // Use Play3D with filename, as we updated the source properties.
                    // Or if we need to ensure we play THAT source, we rely on the filename being the key.
                    audio.sound = soundPlayer.Play3D(audio.filePath, pos, audio.loop);
                }
            }
            else
            {
                audio.sound = soundPlayer.GetEngine()->Play2D(audio.filePath, audio.loop, false /* startPaused */);
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
