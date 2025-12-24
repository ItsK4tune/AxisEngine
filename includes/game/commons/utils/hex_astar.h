#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <game/commons/utils/hex_math.h>

struct HexNode
{
    HexCoord coord;
    float f, g;
};

struct HexNodeCmp
{
    bool operator()(const HexNode &a, const HexNode &b) const
    {
        return a.f > b.f;
    }
};

struct WorldNode
{
    glm::vec3 pos;
    float f, g;
};

struct WorldNodeCmp
{
    bool operator()(const WorldNode &a, const WorldNode &b) const
    {
        return a.f > b.f;
    }
};

class HexAStar
{
public:
    static bool FindPath(
        const HexCoord &start,
        const HexCoord &goal,
        const std::unordered_set<HexCoord> &walkable,
        std::vector<HexCoord> &outPath);

    static bool FindSmoothPath(
        const glm::vec3 &start,
        const glm::vec3 &goal,
        const std::unordered_set<HexCoord> &walkable,
        std::vector<glm::vec3> &outPath);
};
