#pragma once

#include <ecs/unit/media_components.h>
#include <cstdint>

class Animator;

class AnimationGraphRuntime
{
public:
    static AnimationGraphState* FindState(AnimationGraph& graph, uint32_t id);
    static const AnimationGraphState* FindState(const AnimationGraph& graph, uint32_t id);
    static AnimationGraphParameter* FindParameter(AnimationGraph& graph, const std::string& name);
    static bool ConditionsPass(const AnimationGraph& graph, const AnimationGraphTransition& transition);
    static const AnimationGraphTransition* SelectTransition(const AnimationGraph& graph, float normalizedTime);
    static void ConsumeTriggers(AnimationGraph& graph, const AnimationGraphTransition& transition);
    static void Update(AnimationComponent& component, float dt,
                       const glm::mat4& rootTransform = glm::mat4(1.0f));
};

class VFXGraphRuntime
{
public:
    static bool ConditionsPass(const VFXGraph& graph, const VFXGraphLink& link);
    static bool IsNodeActive(const VFXGraph& graph, uint32_t nodeId);
    static void Apply(ParticleEmitterComponent& component);
};
