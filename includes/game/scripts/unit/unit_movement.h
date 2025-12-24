#pragma once

#include <vector>
#include <glm/glm.hpp>

#include <engine/ecs/component.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <game/commons/utils/hex_math.h>

class UnitMovement
{
public:
    float moveSpeed = 10.0f;
    bool isMoving = false;

    void StartHexPath(const std::vector<HexCoord> &pathHex);
    void StartWorldPath(const std::vector<glm::vec3> &pathWorld);
    bool Update(float dt, TransformComponent &trans, RigidBodyComponent *rb);

private:
    std::vector<glm::vec3> m_MovePath;
    int m_PathIndex = 0;
};