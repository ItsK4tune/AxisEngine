#pragma once

#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <game/utils/hex_math.h>

struct UnitStats
{
    int maxHP = 100;
    float currentHP = 100;

    int physicDmg = 20;
    int elementalDmg = 10;
    int attackRange = 1;
    int critChance = 20;

    int defense = 5;
    int resistance = 10;
    int evasion = 10;

    bool isGuarding = false;
    int guardBonus = 50;
    bool isAfflicted = false;
    int synchronizeBonus = 50;

    int moveCost = 2;
    int actionCost = 4;
};

class Unit : public Scriptable
{
public:
    HexCoord gridPos{0, 0, 0};
    int teamID = 1;
    UnitStats stats;

    float moveSpeed = 10.0f;
    bool isMoving = false;
    glm::vec3 targetPos;

    void OnCreate() override;
    void OnUpdate(float dt) override;

    void MoveTo(HexCoord newCoords);
    void Attack(Unit *target);
    void Guard();
    void TakeDamage(int rawPhys, int rawElem, Unit *attacker);
    void Die();

    void ResetState();

private:
    void SyncPhysics(const glm::vec3 &pos);
};