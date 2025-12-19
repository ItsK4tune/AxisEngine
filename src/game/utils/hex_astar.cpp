#include <game/commons/utils/hex_astar.h>
#include <algorithm>

static std::vector<HexCoord> GetNeighbors(const HexCoord& h)
{
    return {
        {h.q + 1, h.r, h.h},
        {h.q - 1, h.r, h.h},
        {h.q, h.r + 1, h.h},
        {h.q, h.r - 1, h.h},
        {h.q + (h.r & 1),     h.r + 1, h.h},
        {h.q + (h.r & 1) - 1, h.r - 1, h.h},
    };
}

bool HexAStar::FindPath(
    const HexCoord& start,
    const HexCoord& goal,
    const std::unordered_set<HexCoord>& walkable,
    std::vector<HexCoord>& outPath)
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

        for (auto& n : GetNeighbors(current.coord))
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
