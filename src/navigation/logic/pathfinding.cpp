#include <navigation/logic/pathfinding.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

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
    if (options.criteria == PathfindingCriteria::StraightLine)
        return {start, end};

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
            const glm::vec3& currentPos = navMesh.nodes[current].position;
            const glm::vec3& neighborPos = navMesh.nodes[neighbor].position;
            float dist = glm::distance(currentPos, neighborPos);
            float weight = 1.0f;

            if (options.criteria == PathfindingCriteria::Smoothest)
            {
                float yDelta = std::abs(currentPos.y - neighborPos.y);
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
            else if (options.criteria == PathfindingCriteria::HighGround)
            {
                float heightReward = glm::clamp(neighborPos.y * 0.12f, 0.0f, 0.65f);
                float climbPenalty = glm::max(0.0f, neighborPos.y - currentPos.y) * 0.08f;
                weight = glm::max(0.25f, 1.0f - heightReward + climbPenalty);
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
    if (options.criteria == PathfindingCriteria::StayOnRoad)
    {
        return glm::distance(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
    }
    return glm::distance(a, b);
}

std::vector<glm::vec3> Pathfinding::FindGridPath(const glm::vec3& start, const glm::vec3& end,
                                                 const NavigationGridComponent& grid,
                                                 const PathfindingOptions& options)
{
    if (options.criteria == PathfindingCriteria::StraightLine)
        return {start, end};
    if (!grid.IsValid())
        return {};

    auto toCell = [&](const glm::vec3& p) {
        int x = static_cast<int>(std::floor((p.x - grid.origin.x) / grid.cellSize));
        int z = static_cast<int>(std::floor((p.z - grid.origin.z) / grid.cellSize));
        x = glm::clamp(x, 0, grid.width - 1);
        z = glm::clamp(z, 0, grid.height - 1);
        return glm::ivec2(x, z);
    };

    auto toWorld = [&](int x, int z) {
        return grid.origin + glm::vec3((static_cast<float>(x) + 0.5f) * grid.cellSize, 0.0f,
                                       (static_cast<float>(z) + 0.5f) * grid.cellSize);
    };

    const glm::ivec2 startCell = toCell(start);
    const glm::ivec2 endCell = toCell(end);
    const int startIndex = grid.Index(startCell.x, startCell.y);
    const int endIndex = grid.Index(endCell.x, endCell.y);

    if (!grid.cells[startIndex].walkable || !grid.cells[endIndex].walkable)
        return {};

    struct GridOpenNode
    {
        int index;
        float gScore;
        float fScore;

        bool operator>(const GridOpenNode& other) const
        {
            return fScore > other.fScore;
        }
    };

    std::priority_queue<GridOpenNode, std::vector<GridOpenNode>, std::greater<GridOpenNode>> openSet;
    std::vector<float> gScore(grid.cells.size(), std::numeric_limits<float>::max());
    std::vector<int> cameFrom(grid.cells.size(), -1);

    gScore[startIndex] = 0.0f;
    openSet.push({startIndex, 0.0f, Heuristic(start, end, options)});

    const glm::ivec2 cardinalDirs[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const glm::ivec2 diagonalDirs[] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    while (!openSet.empty())
    {
        const int current = openSet.top().index;
        openSet.pop();

        if (current == endIndex)
        {
            std::vector<glm::vec3> path;
            path.push_back(end);

            int node = current;
            while (cameFrom[node] != -1)
            {
                int x = node % grid.width;
                int z = node / grid.width;
                path.push_back(toWorld(x, z));
                node = cameFrom[node];
            }

            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        const int cx = current % grid.width;
        const int cz = current / grid.width;
        auto visitNeighbor = [&](int nx, int nz, float stepCost) {
            if (nx < 0 || nz < 0 || nx >= grid.width || nz >= grid.height)
                return;

            const int neighbor = grid.Index(nx, nz);
            const auto& cell = grid.cells[neighbor];
            if (!cell.walkable)
                return;

            float weight = (std::max)(0.01f, cell.cost);
            if (options.criteria == PathfindingCriteria::StayOnRoad)
            {
                for (const auto& tag : options.preferredTags)
                {
                    if (cell.tag == tag)
                    {
                        weight /= (std::max)(0.01f, options.tagWeightBonus);
                        break;
                    }
                }
            }
            else if (options.criteria == PathfindingCriteria::Custom && options.customGridCostFunc)
            {
                weight = options.customGridCostFunc(static_cast<uint32_t>(current), static_cast<uint32_t>(neighbor),
                                                    grid);
            }

            const float tentative = gScore[current] + stepCost * weight;
            if (tentative >= gScore[neighbor])
                return;

            cameFrom[neighbor] = current;
            gScore[neighbor] = tentative;
            const glm::vec3 neighborWorld = toWorld(nx, nz);
            openSet.push({neighbor, tentative, tentative + Heuristic(neighborWorld, end, options)});
        };

        for (const glm::ivec2& d : cardinalDirs)
            visitNeighbor(cx + d.x, cz + d.y, grid.cellSize);

        if (grid.allowDiagonal)
        {
            for (const glm::ivec2& d : diagonalDirs)
                visitNeighbor(cx + d.x, cz + d.y, grid.cellSize * 1.41421356f);
        }
    }

    return {};
}
