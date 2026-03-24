#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <entt/entt.hpp>
#include <cstdint>

struct NavMeshNode
{
    glm::vec3 position;
    std::vector<uint32_t> neighbors;
    uint32_t triangleIndex;
    std::string tag = "walkable";
};

struct NavMeshTriangle
{
    uint32_t indices[3];
    glm::vec3 center;
    glm::vec3 normal;
    std::string tag = "walkable";
};

struct NavMeshComponent
{
    std::vector<glm::vec3> vertices;
    std::vector<NavMeshTriangle> triangles;
    std::vector<NavMeshNode> nodes;

    bool isDynamic = false;
    bool needsRebuild = true;
};
