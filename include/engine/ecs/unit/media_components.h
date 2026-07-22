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
#include <cstdint>
#include <vector>


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

    float volume = 100.0f;
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

enum class AnimationParameterType : uint8_t
{
    Float,
    Bool,
    Trigger
};

struct AnimationGraphParameter
{
    std::string name;
    AnimationParameterType type = AnimationParameterType::Float;
    float floatValue = 0.0f;
    bool boolValue = false;
    bool triggerValue = false;
};

enum class AnimationConditionOp : uint8_t
{
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    IsTrue,
    IsFalse,
    Triggered
};

enum class GraphConditionLogic : uint8_t
{
    And,
    Or,
    Xor,
    Nand,
    Nor,
    Xnor
};

struct AnimationGraphCondition
{
    std::string parameter;
    AnimationConditionOp op = AnimationConditionOp::Greater;
    float threshold = 0.0f;
    bool negated = false;
};

struct AnimationGraphState
{
    uint32_t id = 0;
    std::string name;
    std::string clip;
    float speed = 1.0f;
    glm::vec2 editorPosition = glm::vec2(0.0f);
};

struct AnimationGraphTransition
{
    uint32_t id = 0;
    uint32_t fromState = 0;
    uint32_t toState = 0;
    float duration = 0.2f;
    bool hasExitTime = false;
    float exitTime = 0.9f;
    std::vector<AnimationGraphCondition> conditions;
    GraphConditionLogic conditionLogic = GraphConditionLogic::And;
};

struct AnimationGraph
{
    bool enabled = false;
    uint32_t entryState = 0;
    uint32_t activeState = 0;
    uint32_t nextId = 1;
    std::vector<AnimationGraphParameter> parameters;
    std::vector<AnimationGraphState> states;
    std::vector<AnimationGraphTransition> transitions;
};

struct AnimationComponent
{
    std::vector<std::string> animations;
    float speed = 1.0f;
    float startTime = 0.0f;
    float rate = 0.0f;
    float blendFactor = 0.0f;

    std::shared_ptr<Animator> animator = nullptr;
    std::vector<glm::mat4> boneMatrices;
    AnimationGraph graph;
};

enum class VFXNodeType : uint8_t
{
    Spawn,
    Lifetime,
    Velocity,
    Gravity,
    Drag,
    ColorOverLife,
    SizeOverLife,
    Output
};

struct VFXGraphNode
{
    uint32_t id = 0;
    VFXNodeType type = VFXNodeType::Spawn;
    std::string name;
    glm::vec4 valueA = glm::vec4(0.0f);
    glm::vec4 valueB = glm::vec4(0.0f);
    float scalarA = 0.0f;
    float scalarB = 0.0f;
    bool enabled = true;
    glm::vec2 editorPosition = glm::vec2(0.0f);
};

struct VFXGraphLink
{
    uint32_t id = 0;
    uint32_t fromNode = 0;
    uint32_t toNode = 0;
    std::vector<AnimationGraphCondition> conditions;
    GraphConditionLogic conditionLogic = GraphConditionLogic::And;
};

struct VFXGraph
{
    bool enabled = false;
    uint32_t nextId = 1;
    std::vector<AnimationGraphParameter> parameters;
    std::vector<VFXGraphNode> nodes;
    std::vector<VFXGraphLink> links;
};

struct ParticleEmitterComponent
{
    bool isActive = true;
    ParticleEmitter emitter;
    // How long this entity emits particles; negative values emit indefinitely.
    // ParticleEmitter::LifeTime remains the lifetime of each spawned particle.
    float emissionDuration = -1.0f;
    unsigned int maxParticles = 500;
    std::string textureName = "";
    std::string customShader;
    VFXGraph graph;
};
