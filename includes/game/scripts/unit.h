#pragma once

#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <game/utils/hex_math.h>

class Unit : public Scriptable
{
public:
    HexCoord gridPos{0, 0, 0};
    int teamID = 1;
    float moveSpeed = 10.0f;

    bool isMoving = false;
    glm::vec3 targetPos;

    void OnCreate() override;
    void OnUpdate(float dt) override;

    void MoveTo(HexCoord newCoords);

private:
    void SyncPhysics(const glm::vec3 &pos);
};