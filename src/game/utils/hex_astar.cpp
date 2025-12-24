#include <game/commons/utils/hex_astar.h>
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

static std::vector<glm::vec3> SampleDirections(const glm::vec3 &baseDir)
{
    std::vector<glm::vec3> dirs;
    dirs.push_back(baseDir);
    const int NUM_SAMPLES = 6;
    const float ANGLE_STEP = glm::radians(15.0f);

    for (int i = 1; i <= NUM_SAMPLES; ++i)
    {
        float angle = ANGLE_STEP * i;
        dirs.push_back(glm::rotateY(baseDir, angle));
        dirs.push_back(glm::rotateY(baseDir, -angle));
    }
    return dirs;
}

static std::vector<HexCoord> GetNeighbors(const HexCoord &h)
{
    return {
        {h.q + 1, h.r, h.h},
        {h.q - 1, h.r, h.h},
        {h.q, h.r + 1, h.h},
        {h.q, h.r - 1, h.h},
        {h.q + (h.r & 1), h.r + 1, h.h},
        {h.q + (h.r & 1) - 1, h.r - 1, h.h},
    };
}

bool HexAStar::FindPath(
    const HexCoord &start,
    const HexCoord &goal,
    const std::unordered_set<HexCoord> &walkable,
    std::vector<HexCoord> &outPath)
{
    std::priority_queue<HexNode, std::vector<HexNode>, HexNodeCmp> open;
    std::unordered_map<HexCoord, HexCoord> cameFrom;
    std::unordered_map<HexCoord, float> gScore;

    open.push({start, 0.0f, 0.0f});
    gScore[start] = 0.0f;

    while (!open.empty())
    {
        HexNode current = open.top();
        open.pop();

        if (current.coord == goal)
        {
            outPath.clear();
            HexCoord c = goal;
            while (c != start)
            {
                outPath.push_back(c);
                c = cameFrom[c];
            }
            outPath.push_back(start);
            std::reverse(outPath.begin(), outPath.end());
            return true;
        }

        for (auto &n : GetNeighbors(current.coord))
        {
            if (!walkable.count(n))
                continue;

            float tentativeG = gScore[current.coord] + 1.0f;

            if (!gScore.count(n) || tentativeG < gScore[n])
            {
                cameFrom[n] = current.coord;
                gScore[n] = tentativeG;

                float h = HexMath::Distance(n, goal);
                open.push({n, tentativeG + h, tentativeG});
            }
        }
    }
    return false;
}

bool HexAStar::FindSmoothPath(
    const glm::vec3 &start,
    const glm::vec3 &goal,
    const std::unordered_set<HexCoord> &walkable,
    std::vector<glm::vec3> &outPath)
{
    constexpr float STEP = 0.5f;
    constexpr float GOAL_EPS = 0.4f;
    constexpr float CELL = 0.5f;

    auto hashPos = [&](const glm::vec3 &p)
    {
        int x = (int)std::round(p.x / CELL);
        int z = (int)std::round(p.z / CELL);
        return (x << 16) ^ z;
    };

    std::priority_queue<WorldNode, std::vector<WorldNode>, WorldNodeCmp> open;
    std::unordered_map<int, glm::vec3> cameFrom;
    std::unordered_map<int, float> gScore;
    std::unordered_set<int> closed;

    int startKey = hashPos(start);
    open.push({start, glm::distance(start, goal), 0.0f});
    gScore[startKey] = 0.0f;

    while (!open.empty())
    {
        WorldNode current = open.top();
        open.pop();

        int currentKey = hashPos(current.pos);
        if (closed.count(currentKey))
            continue;
        closed.insert(currentKey);

        if (glm::distance(current.pos, goal) < GOAL_EPS)
        {
            outPath.clear();
            glm::vec3 p = current.pos;
            while (hashPos(p) != startKey)
            {
                outPath.push_back(p);
                p = cameFrom[hashPos(p)];
            }
            outPath.push_back(start);
            std::reverse(outPath.begin(), outPath.end());

            if (!outPath.empty())
                outPath.back() = goal;
            return true;
        }

        glm::vec3 baseDir = glm::normalize(goal - current.pos);
        auto dirs = SampleDirections(baseDir);

        for (auto &d : dirs)
        {
            glm::vec3 next = current.pos + d * STEP;
            HexCoord h = HexMath::WorldToHex(next);

            if (!walkable.count(h))
                continue;

            int key = hashPos(next);
            float tentativeG = current.g + STEP;

            if (!gScore.count(key) || tentativeG < gScore[key])
            {
                cameFrom[key] = current.pos;
                gScore[key] = tentativeG;

                float f = tentativeG + glm::distance(next, goal);
                open.push({next, f, tentativeG});
            }
        }
    }
    return false;
}