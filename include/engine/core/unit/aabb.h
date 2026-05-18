#pragma once

#include <glm/glm.hpp>
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL

struct AABB
{
    glm::vec3 minBound = glm::vec3(0.0f);
    glm::vec3 maxBound = glm::vec3(0.0f);

    AABB() = default;
    AABB(const glm::vec3& min, const glm::vec3& max) : minBound(min), maxBound(max)
    {
    }

    glm::vec3 GetCenter() const
    {
        return (minBound + maxBound) * 0.5f;
    }
    glm::vec3 GetExtent() const
    {
        return (maxBound - minBound) * 0.5f;
    }

    bool Contains(const glm::vec3& point) const
    {
        return (point.x >= minBound.x && point.x <= maxBound.x) && (point.y >= minBound.y && point.y <= maxBound.y) &&
               (point.z >= minBound.z && point.z <= maxBound.z);
    }

    bool Overlaps(const AABB& other) const
    {
        return (minBound.x <= other.maxBound.x && maxBound.x >= other.minBound.x) &&
               (minBound.y <= other.maxBound.y && maxBound.y >= other.minBound.y) &&
               (minBound.z <= other.maxBound.z && maxBound.z >= other.minBound.z);
    }

    void Encapsulate(const glm::vec3& point)
    {
        minBound = (glm::min)(minBound, point);
        maxBound = (glm::max)(maxBound, point);
    }

    void Encapsulate(const AABB& other)
    {
        minBound = (glm::min)(minBound, other.minBound);
        maxBound = (glm::max)(maxBound, other.maxBound);
    }

    AABB Transform(const glm::mat4& matrix) const
    {
        glm::vec3 center = GetCenter();
        glm::vec3 extent = GetExtent();

        glm::vec3 worldCenter = glm::vec3(matrix * glm::vec4(center, 1.0f));

        glm::mat3 rot = glm::mat3(matrix);
        glm::vec3 worldExtent =
            glm::vec3(std::abs(rot[0][0]) * extent.x + std::abs(rot[1][0]) * extent.y + std::abs(rot[2][0]) * extent.z,
                      std::abs(rot[0][1]) * extent.x + std::abs(rot[1][1]) * extent.y + std::abs(rot[2][1]) * extent.z,
                      std::abs(rot[0][2]) * extent.x + std::abs(rot[1][2]) * extent.y + std::abs(rot[2][2]) * extent.z);

        return AABB(worldCenter - worldExtent, worldCenter + worldExtent);
    }

    float DistanceSq(const glm::vec3& point) const
    {
        float dx = (std::max)(minBound.x - point.x, (std::max)(0.0f, point.x - maxBound.x));
        float dy = (std::max)(minBound.y - point.y, (std::max)(0.0f, point.y - maxBound.y));
        float dz = (std::max)(minBound.z - point.z, (std::max)(0.0f, point.z - maxBound.z));
        return dx * dx + dy * dy + dz * dz;
    }

    float Distance(const glm::vec3& point) const
    {
        return std::sqrt(DistanceSq(point));
    }

    glm::vec3 GetSize() const
    {
        return maxBound - minBound;
    }

    static AABB FromCenterSize(const glm::vec3& center, const glm::vec3& size)
    {
        glm::vec3 halfSize = size * 0.5f;
        return AABB(center - halfSize, center + halfSize);
    }
};
