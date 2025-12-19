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

class HexAStar
{
public:
    static bool FindPath(
        const HexCoord &start,
        const HexCoord &goal,
        const std::unordered_set<HexCoord> &walkable,
        std::vector<HexCoord> &outPath);
};
