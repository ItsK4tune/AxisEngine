#include <engine/ecs/component.h>

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 TransformComponent::GetTransformMatrix() const
{
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
    return trans * rot * sca;
}