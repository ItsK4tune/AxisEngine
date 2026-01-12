#include <engine/ecs/system.h>
#include <engine/ecs/component.h>
#include <engine/utils/irrKlang_glm_helpers.h>

void AudioSystem::Update(Scene &scene, SoundManager& soundManager)
{
    auto view = scene.registry.view<AudioSourceComponent>();

    for (auto entity : view)
    {
        auto &audio = view.get<AudioSourceComponent>(entity);

        // Validating init state
        if (audio.playOnAwake && !audio.sound && !audio.shouldPlay)
        {
             // Check if we should start it now (first frame logic usually needs a flag, 
             // but here we just check if handle is null and it's set to play)
             // However, irrklang manages memory.
             audio.shouldPlay = true;
             audio.playOnAwake = false; // Prevent re-triggering logic excessively
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
                TransformComponent* transform = scene.registry.try_get<TransformComponent>(entity);
                glm::vec3 pos = transform ? transform->position : glm::vec3(0.0f);
                
                // We don't have GetSoundSource easily exposed, so we use string path
                // But Play3D needs ISoundSource* ideally for caching, 
                // Let's modify SoundManager to allow string path Play3D or helper
                // For now, assume SoundManager::Play3D accepts file path which isn't true in previous analysis.
                // Let's rely on SoundManager having a way to play or we get the source.
                
                // Correction: SoundManager exposed `Play3D(ISoundSource* ...)`
                // We need to get the source first.
                // Assuming SoundManager has `GetEngine()` exposed publicly.
                
                irrklang::ISoundSource* source = soundManager.GetEngine()->getSoundSource(audio.filePath.c_str());
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

        // Update Position for 3D sounds
        if (audio.is3D && audio.sound && !audio.sound->isFinished())
        {
             TransformComponent* transform = scene.registry.try_get<TransformComponent>(entity);
             if (transform)
             {
                 audio.sound->setPosition(IrrKlangGLMHelpers::convert(transform->position));
             }
        }

        // Cleanup finished sounds
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
