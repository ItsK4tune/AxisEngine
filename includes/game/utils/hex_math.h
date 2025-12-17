#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <vector>

struct HexCoord {
    int q, r;

    bool operator==(const HexCoord& other) const {
        return q == other.q && r == other.r;
    }
};

class HexMath {
public:
    // Kích thước từ tâm đến đỉnh lục giác
    static constexpr float SIZE = 1.0f; 
    
    // Chuyển tọa độ Lưới (q, r) -> Tọa độ Thế giới 3D (x, 0, z)
    // Layout: Pointy-topped (Đỉnh nhọn hướng lên)
    static glm::vec3 HexToWorld(HexCoord c) {
        float x = SIZE * (sqrt(3.0f) * c.q + sqrt(3.0f)/2.0f * c.r);
        float z = SIZE * (3.0f/2.0f * c.r);
        return glm::vec3(x, 0.0f, z);
    }

    // Chuyển tọa độ Thế giới -> Tọa độ Lưới (Dùng khi Click chuột)
    static HexCoord WorldToHex(glm::vec3 pos) {
        float q = (sqrt(3.0f)/3.0f * pos.x - 1.0f/3.0f * pos.z) / SIZE;
        float r = (2.0f/3.0f * pos.z) / SIZE;
        return HexRound(q, r);
    }

private:
    static HexCoord HexRound(float fracQ, float fracR) {
        float fracS = -fracQ - fracR;
        int q = round(fracQ);
        int r = round(fracR);
        int s = round(fracS);
        float q_diff = abs(q - fracQ);
        float r_diff = abs(r - fracR);
        float s_diff = abs(s - fracS);

        if (q_diff > r_diff && q_diff > s_diff) q = -r - s;
        else if (r_diff > s_diff) r = -q - s;
        return {q, r};
    }
};