#include <physics/unit/ray.h>

namespace RaycastUtils
{
Ray CalculateRay(const glm::vec2& screenPos, const glm::vec2& viewportSize, const glm::mat4& view,
                 const glm::mat4& proj)
{
    float x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;
    float z = 1.0f;
    glm::vec3 ray_nds = glm::vec3(x, y, z);

    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f);

    glm::vec4 ray_eye = glm::inverse(proj) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    glm::vec3 origin = glm::vec3(glm::inverse(view)[3]);

    return {origin, ray_wor};
}

glm::vec3 AngleToDirection(float yaw, float pitch)
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(direction);
}
}  // namespace RaycastUtils
