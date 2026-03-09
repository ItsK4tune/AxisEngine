#include <navigation/logic/pathfinding.h>
#include <queue>
#include <unordered_map>
#include <algorithm>

struct AStarNode {
    uint32_t index;
    float gScore;
    float fScore;

    bool operator>(const AStarNode& other) const {
        return fScore > other.fScore;
    }
};

std::vector<glm::vec3> Pathfinding::FindPath(const glm::vec3& start, const glm::vec3& end, const NavMeshComponent& navMesh)
{
    if (navMesh.nodes.empty()) return {};

    uint32_t startNodeIdx = FindClosestNode(start, navMesh);
    uint32_t endNodeIdx = FindClosestNode(end, navMesh);

    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    std::unordered_map<uint32_t, uint32_t> cameFrom;
    std::unordered_map<uint32_t, float> gScore;

    gScore[startNodeIdx] = 0.0f;
    openSet.push({startNodeIdx, 0.0f, Heuristic(navMesh.nodes[startNodeIdx].position, end)});

    while (!openSet.empty()) {
        uint32_t current = openSet.top().index;
        openSet.pop();

        if (current == endNodeIdx) {
            std::vector<glm::vec3> path;
            path.push_back(end);
            while (cameFrom.count(current)) {
                path.push_back(navMesh.nodes[current].position);
                current = cameFrom[current];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (uint32_t neighbor : navMesh.nodes[current].neighbors) {
            float tentative_gScore = gScore[current] + glm::distance(navMesh.nodes[current].position, navMesh.nodes[neighbor].position);
            
            if (!gScore.count(neighbor) || tentative_gScore < gScore[neighbor]) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_gScore;
                float fScore = tentative_gScore + Heuristic(navMesh.nodes[neighbor].position, end);
                openSet.push({neighbor, tentative_gScore, fScore});
            }
        }
    }

    return {}; // No path found
}

uint32_t Pathfinding::FindClosestNode(const glm::vec3& pos, const NavMeshComponent& navMesh)
{
    uint32_t closest = 0;
    float minDist = glm::distance(pos, navMesh.nodes[0].position);

    for (uint32_t i = 1; i < (uint32_t)navMesh.nodes.size(); ++i) {
        float d = glm::distance(pos, navMesh.nodes[i].position);
        if (d < minDist) {
            minDist = d;
            closest = i;
        }
    }
    return closest;
}

float Pathfinding::Heuristic(const glm::vec3& a, const glm::vec3& b)
{
    return glm::distance(a, b);
}
