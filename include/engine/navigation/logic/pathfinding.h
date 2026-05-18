#pragma once

#include <navigation/unit/navmesh_component.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

enum class PathfindingCriteria
{
    Shortest,
    Smoothest,
    StayOnRoad,
    Custom
};

struct PathfindingOptions
{
    PathfindingCriteria criteria = PathfindingCriteria::Shortest;

    std::vector<std::string> preferredTags = {"walkable"};
    float tagWeightBonus = 5.0f;

    float altitudePenaltyWeight = 10.0f;

    std::function<float(uint32_t current, uint32_t neighbor, const NavMeshComponent& navMesh)> customCostFunc;
    std::function<float(const glm::vec3& a, const glm::vec3& b)> customHeuristicFunc;
};

class Pathfinding
{
public:
    static std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end,
                                           const NavMeshComponent& navMesh,
                                           const PathfindingOptions& options = PathfindingOptions());

private:
    static uint32_t FindClosestNode(const glm::vec3& pos, const NavMeshComponent& navMesh);
    static float Heuristic(const glm::vec3& a, const glm::vec3& b, const PathfindingOptions& options);
};
