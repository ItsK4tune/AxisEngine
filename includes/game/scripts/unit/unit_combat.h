#pragma once

#include <game/scripts/unit/unit_data.h>

struct DamageResult
{
    int finalPhys;
    int finalElem;
    int finalDamage;
    bool isCrit;
    bool isMiss;
    bool isSync;
};

class UnitCombat
{
public:
    DamageResult CalculateDamage(const UnitStats &attacker, const UnitStats &defender, const UnitState &defenderState);
};