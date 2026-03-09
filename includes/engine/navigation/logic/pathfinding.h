#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <navigation/unit/navmesh_component.h>

class Pathfinding
{
public:
    static std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end, const NavMeshComponent& navMesh);

private:
    static uint32_t FindClosestNode(const glm::vec3& pos, const NavMeshComponent& navMesh);
    static float Heuristic(const glm::vec3& a, const glm::vec3& b);
};
