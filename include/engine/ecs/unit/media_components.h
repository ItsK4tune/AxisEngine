#pragma once

#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <render/logic/particle_emitter.h>
#include <render/logic/video_decoder.h>
#include <resource/unit/animation.h>
#include <resource/unit/animator.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL

struct AudioSourceComponent
{
    std::shared_ptr<ISound> sound = nullptr;
    std::shared_ptr<IAudioSource> source = nullptr;
    std::string filePath = "";
    std::string resourceName = "";

    bool playOnAwake = false;
    bool shouldPlay = false;
    bool loop = false;
    bool is3D = false;

    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    float speed = 1.0f;

    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    glm::vec3 velocity = glm::vec3(0.0f);
};

struct VideoPlayerComponent
{
    std::string filePath = "";
    bool isLooping = false;
    bool playOnAwake = false;
    float volume = 1.0f;
    bool isPlaying = false;
    bool isLoaded = false;
    float speed = 1.0f;
    int maxDecodes = 1;

    std::shared_ptr<VideoDecoder> decoder = nullptr;
};

// ─── Free functions for VideoPlayerComponent (ECS-compliant) ───

inline void PlayVideo(VideoPlayerComponent& vp)
{
    vp.isPlaying = true;
    if (vp.decoder)
        vp.decoder->Play();
}
inline void PauseVideo(VideoPlayerComponent& vp)
{
    vp.isPlaying = false;
    if (vp.decoder)
        vp.decoder->Pause();
}
inline void StopVideo(VideoPlayerComponent& vp)
{
    vp.isPlaying = false;
    if (vp.decoder)
        vp.decoder->Stop();
}
inline void ReplayVideo(VideoPlayerComponent& vp)
{
    vp.isPlaying = true;
    if (vp.decoder)
    {
        vp.decoder->Seek(0);
        vp.decoder->Play();
    }
}
inline void SeekVideo(VideoPlayerComponent& vp, double time)
{
    if (vp.decoder)
        vp.decoder->Seek(time);
}

struct AnimationComponent
{
    std::vector<std::string> animations;
    float speed = 1.0f;
    float startTime = 0.0f;
    float rate = 0.0f;
    float blendFactor = 0.0f;

    std::shared_ptr<Animator> animator = nullptr;
    std::vector<glm::mat4> boneMatrices;
};

struct ParticleEmitterComponent
{
    bool isActive = true;
    ParticleEmitter emitter;
    float emissionRate = 10.0f;
    float lifetime = 2.0f;
    float speed = 1.0f;
    float size = 0.1f;
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float spread = 0.3f;
    glm::vec4 startColor = glm::vec4(1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    std::string textureName = "";
    std::string customShader;
};
