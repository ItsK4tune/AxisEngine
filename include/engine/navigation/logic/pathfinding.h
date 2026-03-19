#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <glm/glm.hpp>
#include <navigation/unit/navmesh_component.h>

enum class PathfindingCriteria
{
    Shortest,       // Standard Euclidean
    Smoothest,      // Prefers minimal altitude change
    StayOnRoad,     // Prefers nodes with "road" or preferred tags
    Custom          // Uses user-provided callback
};

struct PathfindingOptions
{
    PathfindingCriteria criteria = PathfindingCriteria::Shortest;
    
    // For StayOnRoad or PreferTags
    std::vector<std::string> preferredTags = { "walkable" };
    float tagWeightBonus = 5.0f; // Multiplier to reduce cost for preferred tags

    // For Smoothest
    float altitudePenaltyWeight = 10.0f;

    // For Custom
    std::function<float(uint32_t current, uint32_t neighbor, const NavMeshComponent& navMesh)> customCostFunc;
    std::function<float(const glm::vec3& a, const glm::vec3& b)> customHeuristicFunc;
};

class Pathfinding
{
public:
    static std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end, const NavMeshComponent& navMesh, const PathfindingOptions& options = PathfindingOptions());

private:
    static uint32_t FindClosestNode(const glm::vec3& pos, const NavMeshComponent& navMesh);
    static float Heuristic(const glm::vec3& a, const glm::vec3& b, const PathfindingOptions& options);
};
