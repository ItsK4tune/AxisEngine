#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <render/logic/video_decoder.h>
#include <render/logic/animation.h>
#include <render/logic/animator.h>
#include <render/logic/particle_emitter.h>

#define GLM_ENABLE_EXPERIMENTAL

// --- Audio ---

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

// --- Video ---

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

    void Play();
    void Pause();
    void Stop();
    void Replay();
    void Seek(double time);
};

// --- Animation ---

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

// --- Particle ---

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
};
