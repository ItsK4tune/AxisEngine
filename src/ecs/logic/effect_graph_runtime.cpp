#include <ecs/logic/effect_graph_runtime.h>
#include <resource/unit/animator.h>
#include <algorithm>
#include <cmath>
#include <unordered_set>

AnimationGraphState* AnimationGraphRuntime::FindState(AnimationGraph& graph, uint32_t id)
{
    auto it = std::find_if(graph.states.begin(), graph.states.end(),
                           [id](const auto& state) { return state.id == id; });
    return it == graph.states.end() ? nullptr : &*it;
}

const AnimationGraphState* AnimationGraphRuntime::FindState(const AnimationGraph& graph, uint32_t id)
{
    auto it = std::find_if(graph.states.begin(), graph.states.end(),
                           [id](const auto& state) { return state.id == id; });
    return it == graph.states.end() ? nullptr : &*it;
}

AnimationGraphParameter* AnimationGraphRuntime::FindParameter(AnimationGraph& graph, const std::string& name)
{
    auto it = std::find_if(graph.parameters.begin(), graph.parameters.end(),
                           [&name](const auto& parameter) { return parameter.name == name; });
    return it == graph.parameters.end() ? nullptr : &*it;
}

namespace
{
const AnimationGraphParameter* FindParameterConst(const AnimationGraph& graph, const std::string& name)
{
    auto it = std::find_if(graph.parameters.begin(), graph.parameters.end(),
                           [&name](const auto& parameter) { return parameter.name == name; });
    return it == graph.parameters.end() ? nullptr : &*it;
}

const AnimationGraphParameter* FindParameterConst(const VFXGraph& graph, const std::string& name)
{
    auto it = std::find_if(graph.parameters.begin(), graph.parameters.end(),
                           [&name](const auto& parameter) { return parameter.name == name; });
    return it == graph.parameters.end() ? nullptr : &*it;
}

bool ConditionPasses(const AnimationGraphParameter& parameter, const AnimationGraphCondition& condition)
{
    constexpr float epsilon = 0.0001f;
    bool result = false;
    switch (condition.op)
    {
        case AnimationConditionOp::Greater: result = parameter.floatValue > condition.threshold; break;
        case AnimationConditionOp::GreaterEqual: result = parameter.floatValue >= condition.threshold; break;
        case AnimationConditionOp::Less: result = parameter.floatValue < condition.threshold; break;
        case AnimationConditionOp::LessEqual: result = parameter.floatValue <= condition.threshold; break;
        case AnimationConditionOp::Equal:
            result = parameter.type == AnimationParameterType::Float
                         ? std::abs(parameter.floatValue - condition.threshold) <= epsilon
                         : parameter.boolValue == (condition.threshold != 0.0f);
            break;
        case AnimationConditionOp::NotEqual:
            result = parameter.type == AnimationParameterType::Float
                         ? std::abs(parameter.floatValue - condition.threshold) > epsilon
                         : parameter.boolValue != (condition.threshold != 0.0f);
            break;
        case AnimationConditionOp::IsTrue: result = parameter.boolValue; break;
        case AnimationConditionOp::IsFalse: result = !parameter.boolValue; break;
        case AnimationConditionOp::Triggered: result = parameter.triggerValue; break;
    }
    return condition.negated ? !result : result;
}

template <typename GraphT>
bool ConditionsPassImpl(const GraphT& graph, const std::vector<AnimationGraphCondition>& conditions,
                        GraphConditionLogic logic)
{
    if (conditions.empty())
        return true;

    size_t passed = 0;
    for (const auto& condition : conditions)
    {
        const auto* parameter = FindParameterConst(graph, condition.parameter);
        if (!parameter)
            return false;
        if (ConditionPasses(*parameter, condition))
            ++passed;
    }

    const bool all = passed == conditions.size();
    const bool any = passed != 0;
    const bool oddParity = (passed % 2) != 0;
    switch (logic)
    {
        case GraphConditionLogic::And: return all;
        case GraphConditionLogic::Or: return any;
        case GraphConditionLogic::Xor: return oddParity;
        case GraphConditionLogic::Nand: return !all;
        case GraphConditionLogic::Nor: return !any;
        case GraphConditionLogic::Xnor: return !oddParity;
    }
    return false;
}
}

bool AnimationGraphRuntime::ConditionsPass(const AnimationGraph& graph,
                                           const AnimationGraphTransition& transition)
{
    return ConditionsPassImpl(graph, transition.conditions, transition.conditionLogic);
}

const AnimationGraphTransition* AnimationGraphRuntime::SelectTransition(const AnimationGraph& graph,
                                                                        float normalizedTime)
{
    for (const auto& transition : graph.transitions)
    {
        if (transition.fromState != graph.activeState)
            continue;
        if (transition.hasExitTime && normalizedTime < transition.exitTime)
            continue;
        if (ConditionsPass(graph, transition))
            return &transition;
    }
    return nullptr;
}

void AnimationGraphRuntime::ConsumeTriggers(AnimationGraph& graph, const AnimationGraphTransition& transition)
{
    for (const auto& condition : transition.conditions)
    {
        if (condition.op != AnimationConditionOp::Triggered || condition.negated)
            continue;
        if (auto* parameter = FindParameter(graph, condition.parameter))
            if (parameter->triggerValue)
                parameter->triggerValue = false;
    }
}

void AnimationGraphRuntime::Update(AnimationComponent& component, float dt, const glm::mat4& rootTransform)
{
    if (!component.animator)
        return;

    auto& graph = component.graph;
    if (graph.enabled && !graph.states.empty())
    {
        if (!FindState(graph, graph.activeState))
        {
            graph.activeState = FindState(graph, graph.entryState) ? graph.entryState : graph.states.front().id;
            if (const auto* entry = FindState(graph, graph.activeState); entry && !entry->clip.empty())
                component.animator->PlayAnimation(entry->clip);
        }

        auto* state = FindState(graph, graph.activeState);
        if (state)
        {
            component.animator->SetSpeed(component.speed * state->speed);
            const float duration = component.animator->GetDuration();
            const float normalizedTime = duration > 0.0f ? component.animator->GetCurrentTime() / duration : 0.0f;
            if (!component.animator->IsCrossFading())
            {
                if (const auto* transition = SelectTransition(graph, normalizedTime))
                {
                    if (const auto* target = FindState(graph, transition->toState); target && !target->clip.empty())
                    {
                        component.animator->CrossFade(target->clip, transition->duration);
                        ConsumeTriggers(graph, *transition);
                        graph.activeState = target->id;
                    }
                }
            }
        }
    }
    else
    {
        component.animator->SetSpeed(component.speed);
    }

    component.animator->SetUpdateRate(component.rate);
    component.animator->UpdateAnimation(dt, rootTransform);
}

bool VFXGraphRuntime::ConditionsPass(const VFXGraph& graph, const VFXGraphLink& link)
{
    return ConditionsPassImpl(graph, link.conditions, link.conditionLogic);
}

bool VFXGraphRuntime::IsNodeActive(const VFXGraph& graph, uint32_t nodeId)
{
    if (graph.links.empty())
        return true;

    std::unordered_set<uint32_t> active;
    for (const auto& node : graph.nodes)
        if (node.type == VFXNodeType::Output && node.enabled)
            active.insert(node.id);

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto& link : graph.links)
        {
            if (active.contains(link.toNode) && ConditionsPass(graph, link) && active.insert(link.fromNode).second)
                changed = true;
        }
    }
    return active.contains(nodeId);
}

void VFXGraphRuntime::Apply(ParticleEmitterComponent& component)
{
    auto& graph = component.graph;
    if (!graph.enabled)
        return;

    for (const auto& node : graph.nodes)
    {
        if (!node.enabled || !IsNodeActive(graph, node.id))
            continue;
        switch (node.type)
        {
            case VFXNodeType::Spawn: component.emitter.SpawnRate = std::max(0.0f, node.scalarA); break;
            case VFXNodeType::Lifetime: component.emitter.LifeTime = std::max(0.001f, node.scalarA); break;
            case VFXNodeType::Velocity:
                component.emitter.MinVelocity = glm::vec3(node.valueA);
                component.emitter.MaxVelocity = glm::vec3(node.valueB);
                break;
            case VFXNodeType::Gravity: component.emitter.Gravity = glm::vec3(node.valueA); break;
            case VFXNodeType::Drag: component.emitter.Drag = std::max(0.0f, node.scalarA); break;
            case VFXNodeType::ColorOverLife:
                component.emitter.StartColor = node.valueA;
                component.emitter.EndColor = node.valueB;
                break;
            case VFXNodeType::SizeOverLife:
                component.emitter.StartSize = std::max(0.0f, node.scalarA);
                component.emitter.EndSize = std::max(0.0f, node.scalarB);
                break;
            case VFXNodeType::Output: break;
        }
    }

    for (const auto& link : graph.links)
    {
        if (!IsNodeActive(graph, link.fromNode) || !IsNodeActive(graph, link.toNode) || !ConditionsPass(graph, link))
            continue;
        for (const auto& condition : link.conditions)
        {
            if (condition.op != AnimationConditionOp::Triggered || condition.negated)
                continue;
            auto parameter = std::find_if(graph.parameters.begin(), graph.parameters.end(),
                                          [&condition](const auto& item) { return item.name == condition.parameter; });
            if (parameter != graph.parameters.end() && parameter->triggerValue)
                parameter->triggerValue = false;
        }
    }
}
