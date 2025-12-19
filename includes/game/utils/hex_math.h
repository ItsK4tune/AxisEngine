#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <vector>

struct HexCoord
{
    int q, r, h;

    bool operator==(const HexCoord &other) const
    {
        return q == other.q && r == other.r && h == other.h;
    }
};

class HexMath
{
public:
    static constexpr float SIZE = 1.0f;
    static constexpr float HEIGHT_STEP = 0.5f;

    static glm::vec3 HexToWorld(HexCoord c)
    {
        float x = SIZE * (sqrt(3.0f) * c.q + sqrt(3.0f) / 2.0f * c.r);
        float z = SIZE * (3.0f / 2.0f * c.r);
        float y = c.h * HEIGHT_STEP;
        return glm::vec3(x, y, z);
    }

    static HexCoord WorldToHex(glm::vec3 pos)
    {
        float q = (sqrt(3.0f) / 3.0f * pos.x - 1.0f / 3.0f * pos.z) / SIZE;
        float r = (2.0f / 3.0f * pos.z) / SIZE;

        int h = static_cast<int>(std::round(pos.y / HEIGHT_STEP));

        HexCoord coord = HexRound(q, r);
        coord.h = h;
        return coord;
    }

    static int Distance(HexCoord a, HexCoord b)
    {
        int dist2D = (abs(a.q - b.q) + abs(a.q + a.r - b.q - b.r) + abs(a.r - b.r)) / 2;
        return dist2D + abs(a.h - b.h);
    }

private:
    static HexCoord HexRound(float fracQ, float fracR)
    {
        float fracS = -fracQ - fracR;
        int q = (int)round(fracQ);
        int r = (int)round(fracR);
        int s = (int)round(fracS);
        float q_diff = abs(q - fracQ);
        float r_diff = abs(r - fracR);
        float s_diff = abs(s - fracS);

        if (q_diff > r_diff && q_diff > s_diff)
            q = -r - s;
        else if (r_diff > s_diff)
            r = -q - s;
        return {q, r, 0};
    }
};