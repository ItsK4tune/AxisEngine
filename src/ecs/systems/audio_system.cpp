#include <ecs/system.h>
#include <ecs/component.h>
#include <utils/irrKlang_glm_helpers.h>

void AudioSystem::Update(Scene &scene, SoundManager &soundManager)
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
                audio.sound->stop();
                audio.sound->drop();
                audio.sound = nullptr;
            }

            if (audio.is3D)
            {
                TransformComponent *transform = scene.registry.try_get<TransformComponent>(entity);
                glm::vec3 pos = transform ? transform->position : glm::vec3(0.0f);

                irrklang::ISoundSource *source = soundManager.GetEngine()->getSoundSource(audio.filePath.c_str());
                if (!source)
                    source = soundManager.GetEngine()->addSoundSourceFromFile(audio.filePath.c_str());

                if (source)
                {
                    source->setDefaultMinDistance(audio.minDistance);
                    audio.sound = soundManager.Play3D(source, pos, audio.loop);
                }
            }
            else
            {
                audio.sound = soundManager.GetEngine()->play2D(audio.filePath.c_str(), audio.loop, false, true);
            }

            if (audio.sound)
            {
                audio.sound->setVolume(audio.volume);
            }
        }

        if (audio.is3D && audio.sound && !audio.sound->isFinished())
        {
            TransformComponent *transform = scene.registry.try_get<TransformComponent>(entity);
            if (transform)
            {
                audio.sound->setPosition(IrrKlangGLMHelpers::convert(transform->position));
            }
        }

        if (audio.sound && audio.sound->isFinished())
        {
            audio.sound->drop();
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
            audio.sound->stop();
            audio.sound->drop();
            audio.sound = nullptr;
        }
    }
}
