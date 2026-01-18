#include <ecs/component.h>
#include <graphic/video_decoder.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <vector>
#include <algorithm>

glm::mat4 TransformComponent::GetLocalModelMatrix() const
{
    if (position != m_LastPosition || rotation != m_LastRotation || scale != m_LastScale)
    {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rot = glm::mat4_cast(rotation);
        glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
        
        m_LocalMatrix = trans * rot * sca;
        
        m_LastPosition = position;
        m_LastRotation = rotation;
        m_LastScale = scale;
        
        m_Version++;
    }
    return m_LocalMatrix;
}

glm::mat4 TransformComponent::GetWorldModelMatrix(entt::registry& registry) const
{
    GetLocalModelMatrix();
    
    bool parentChanged = false;
    if (parent != m_LastParent)
    {
        m_LastParent = parent;
        parentChanged = true;
    }

    if (registry.valid(parent) && parent != entt::null)
    {
        if (registry.all_of<TransformComponent>(parent))
        {
            const auto& parentTrans = registry.get<TransformComponent>(parent);
            glm::mat4 parentWorld = parentTrans.GetWorldModelMatrix(registry); 
            uint32_t parentVer = parentTrans.GetVersion(); 

            if (parentChanged || parentVer != m_LastParentVersion || m_Version != m_LastLocalVersion)
            {
                m_WorldMatrix = parentWorld * m_LocalMatrix;
                m_LastParentVersion = parentVer;
                m_LastLocalVersion = m_Version;
            }
        }
        else
        {
             // Parent has no transform? Treat as root.
             m_WorldMatrix = m_LocalMatrix;
        }
    }
    else
    {
        m_WorldMatrix = m_LocalMatrix;
    }
    
    return m_WorldMatrix;
}

void TransformComponent::SetParent(entt::entity thisEntity, entt::entity newParent, entt::registry& registry, bool keepWorldTransform)
{
    if (thisEntity == newParent || parent == newParent) return;

    glm::mat4 worldMatrix;
    if (keepWorldTransform)
    {
        worldMatrix = GetWorldModelMatrix(registry);
    }

    if (registry.valid(parent) && registry.all_of<TransformComponent>(parent))
    {
        auto& oldParentTrans = registry.get<TransformComponent>(parent);
        oldParentTrans.RemoveChild(thisEntity);
    }

    parent = newParent;

    if (registry.valid(newParent) && registry.all_of<TransformComponent>(newParent))
    {
        auto& newParentTrans = registry.get<TransformComponent>(newParent);
        newParentTrans.children.push_back(thisEntity);

        if (keepWorldTransform)
        {
            glm::mat4 parentWorldMatrix = newParentTrans.GetWorldModelMatrix(registry);
            glm::mat4 newLocalMatrix = glm::inverse(parentWorldMatrix) * worldMatrix;

            glm::vec3 s;
            glm::quat r;
            glm::vec3 t;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(newLocalMatrix, s, r, t, skew, perspective);

            position = t;
            rotation = r;
            scale = s;
        }
    }
    else if (keepWorldTransform)
    {
        glm::vec3 s;
        glm::quat r;
        glm::vec3 t;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldMatrix, s, r, t, skew, perspective);

        position = t;
        rotation = r;
        scale = s;
    }
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