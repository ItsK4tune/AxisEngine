#include <algorithm>
#include <ecs/component.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <rendering/core/video_decoder.h>
#include <iostream>
#include <core/utils/logger.h>
#include <vector>

glm::mat4 TransformComponent::GetLocalModelMatrix() const
{
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
    return trans * rot * sca;
}

glm::mat4 TransformComponent::GetWorldModelMatrix(entt::registry& registry) const
{
    glm::mat4 local = GetLocalModelMatrix();
    if (registry.valid(parent) && parent != entt::null && registry.all_of<TransformComponent>(parent))
    {
        return registry.get<TransformComponent>(parent).GetWorldModelMatrix(registry) * local;
    }
    return local;
}

glm::mat4 TransformComponent::GetInterpolatedLocalMatrix(float alpha) const
{
    glm::vec3 p = glm::mix(prevPosition, position, alpha);
    glm::quat r = glm::slerp(prevRotation, rotation, alpha);
    glm::vec3 s = glm::mix(prevScale, scale, alpha);
    return glm::translate(glm::mat4(1.0f), p) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
}

glm::mat4 TransformComponent::GetInterpolatedWorldMatrix(entt::registry& registry, float alpha) const
{
    glm::mat4 local = GetInterpolatedLocalMatrix(alpha);
    if (registry.valid(parent) && parent != entt::null && registry.all_of<TransformComponent>(parent))
    {
        return registry.get<TransformComponent>(parent).GetInterpolatedWorldMatrix(registry, alpha) * local;
    }
    return local;
}

void TransformComponent::SetDirty(entt::registry &registry)
{
    // Legacy dirty marking
}

void TransformComponent::SetParent(entt::entity thisEntity, entt::entity newParent, entt::registry& registry, bool keepWorldTransform)
{
    // Minimal legacy implementation
    parent = newParent;
}

void TransformComponent::AddChild(entt::entity thisEntity, entt::entity child, entt::registry& registry, bool keepWorldTransform)
{
    if (registry.valid(child) && registry.all_of<TransformComponent>(child))
    {
        auto& childTrans = registry.get<TransformComponent>(child);
        childTrans.SetParent(child, thisEntity, registry, keepWorldTransform);
    }
}

void TransformComponent::RemoveChild(entt::entity child)
{
     children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void VideoPlayerComponent::Play()
{
    isPlaying = true;
    if (decoder) decoder->Play();
}

void VideoPlayerComponent::Pause()
{
    isPlaying = false;
    if (decoder) decoder->Pause();
}

void VideoPlayerComponent::Stop()
{
    isPlaying = false;
    if (decoder) decoder->Stop();
}

void VideoPlayerComponent::Replay()
{
    isPlaying = true;
    if (decoder)
    {
        decoder->Seek(0);
        decoder->Play();
    }
}

void VideoPlayerComponent::Seek(double time)
{
    if (decoder) decoder->Seek(time);
}
