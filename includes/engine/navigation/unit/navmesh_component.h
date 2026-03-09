#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <entt/entt.hpp>

struct NavMeshNode
{
    glm::vec3 position;
    std::vector<uint32_t> neighbors; // Indices into nodes vector
    uint32_t triangleIndex;
};

struct NavMeshTriangle
{
    uint32_t indices[3]; // Indices into vertices vector
    glm::vec3 center;
    glm::vec3 normal;
};

struct NavMeshComponent
{
    std::vector<glm::vec3> vertices;
    std::vector<NavMeshTriangle> triangles;
    std::vector<NavMeshNode> nodes; // Pathfinding nodes (usually triangle centers)

    bool isDynamic = false;
    bool needsRebuild = true;
};
