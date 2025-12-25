#pragma once

#include <game/commons/utils/hex_math.h>

class Team;

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

    int guardBonus = 50;
    int synchronizeBonus = 50;

    int moveRadius = 4;
    int lightRadius = 2;

    int moveCost = 2;
    int actionCost = 4;
};

struct UnitState
{
    HexCoord gridPos{0, 0, 0};
    Team* team = nullptr;
    bool isGuarding = false;
    bool isAfflicted = false;
    bool isDead = false;
    bool wantToUseSkill = false;
};