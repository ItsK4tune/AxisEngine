#include <ecs/component.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <vector>
#include <algorithm>

glm::mat4 TransformComponent::GetLocalModelMatrix() const
{
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
    return trans * rot * sca;
}

glm::mat4 TransformComponent::GetWorldModelMatrix(entt::registry& registry) const
{
    glm::mat4 model = GetLocalModelMatrix();
    
    if (registry.valid(parent) && parent != entt::null)
    {
        if (registry.all_of<TransformComponent>(parent))
        {
            const auto& parentTrans = registry.get<TransformComponent>(parent);
            return parentTrans.GetWorldModelMatrix(registry) * model;
        }
    }
    
    return model;
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