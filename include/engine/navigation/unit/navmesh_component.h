#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

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

    int terrainGridResolution = 64;
    float walkableNormalY = 0.3f;
    float carveHeightPadding = 0.5f;
    float carveAgentRadius = 0.0f;
};

struct NavigationGridCell
{
    bool walkable = true;
    float cost = 1.0f;
    std::string tag = "walkable";
};

struct NavigationGridComponent
{
    glm::vec3 origin = glm::vec3(0.0f);
    int width = 0;
    int height = 0;
    float cellSize = 1.0f;
    bool allowDiagonal = false;
    std::vector<NavigationGridCell> cells;

    bool IsValid() const
    {
        return width > 0 && height > 0 && cellSize > 0.0f &&
               cells.size() == static_cast<size_t>(width * height);
    }

    int Index(int x, int z) const
    {
        return z * width + x;
    }
};
