#include <navigation/logic/pathfinding.h>
#include <algorithm>
#include <queue>
#include <unordered_map>

struct AStarNode
{
    uint32_t index;
    float gScore;
    float fScore;

    bool operator>(const AStarNode& other) const
    {
        return fScore > other.fScore;
    }
};

std::vector<glm::vec3> Pathfinding::FindPath(const glm::vec3& start, const glm::vec3& end,
                                             const NavMeshComponent& navMesh, const PathfindingOptions& options)
{
    if (navMesh.nodes.empty())
        return {};

    uint32_t startNodeIdx = FindClosestNode(start, navMesh);
    uint32_t endNodeIdx = FindClosestNode(end, navMesh);

    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
    std::unordered_map<uint32_t, uint32_t> cameFrom;
    std::unordered_map<uint32_t, float> gScore;

    gScore[startNodeIdx] = 0.0f;
    openSet.push({startNodeIdx, 0.0f, Heuristic(navMesh.nodes[startNodeIdx].position, end, options)});

    while (!openSet.empty())
    {
        uint32_t current = openSet.top().index;
        openSet.pop();

        if (current == endNodeIdx)
        {
            std::vector<glm::vec3> path;
            path.push_back(end);
            while (cameFrom.count(current))
            {
                path.push_back(navMesh.nodes[current].position);
                current = cameFrom[current];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (uint32_t neighbor : navMesh.nodes[current].neighbors)
        {
            float dist = glm::distance(navMesh.nodes[current].position, navMesh.nodes[neighbor].position);
            float weight = 1.0f;

            if (options.criteria == PathfindingCriteria::Smoothest)
            {
                float yDelta = std::abs(navMesh.nodes[current].position.y - navMesh.nodes[neighbor].position.y);
                weight = 1.0f + (yDelta * options.altitudePenaltyWeight);
            }
            else if (options.criteria == PathfindingCriteria::StayOnRoad)
            {
                bool onRoad = false;
                for (const auto& tag : options.preferredTags)
                {
                    if (navMesh.nodes[neighbor].tag == tag)
                    {
                        onRoad = true;
                        break;
                    }
                }
                if (onRoad)
                    weight = 1.0f / options.tagWeightBonus;
            }
            else if (options.criteria == PathfindingCriteria::OnlyXZ)
            {
                float yDelta = std::abs(navMesh.nodes[current].position.y - navMesh.nodes[neighbor].position.y);
                weight = 1.0f + (yDelta * options.altitudePenaltyWeight * 2.0f);
            }
            else if (options.criteria == PathfindingCriteria::OnlyY)
            {
                float yDelta = std::abs(navMesh.nodes[current].position.y - navMesh.nodes[neighbor].position.y);
                weight = 1.0f + (1.0f - glm::clamp(yDelta, 0.0f, 1.0f)) * 2.0f;
            }
            else if (options.criteria == PathfindingCriteria::Custom && options.customCostFunc)
            {
                weight = options.customCostFunc(current, neighbor, navMesh);
            }

            float tentative_gScore = gScore[current] + (dist * weight);

            if (!gScore.count(neighbor) || tentative_gScore < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_gScore;
                float fScore = tentative_gScore + Heuristic(navMesh.nodes[neighbor].position, end, options);
                openSet.push({neighbor, tentative_gScore, fScore});
            }
        }
    }

    return {};
}

uint32_t Pathfinding::FindClosestNode(const glm::vec3& pos, const NavMeshComponent& navMesh)
{
    if (navMesh.nodes.empty())
        return 0;

    uint32_t closest = 0;
    float minDist = glm::distance(pos, navMesh.nodes[0].position);

    for (uint32_t i = 1; i < (uint32_t)navMesh.nodes.size(); ++i)
    {
        float d = glm::distance(pos, navMesh.nodes[i].position);
        if (d < minDist)
        {
            minDist = d;
            closest = i;
        }
    }
    return closest;
}

float Pathfinding::Heuristic(const glm::vec3& a, const glm::vec3& b, const PathfindingOptions& options)
{
    if (options.criteria == PathfindingCriteria::Custom && options.customHeuristicFunc)
    {
        return options.customHeuristicFunc(a, b);
    }
    return glm::distance(a, b);
}
