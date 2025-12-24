#include <game/scripts/unit/unit_combat.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

DamageResult UnitCombat::CalculateDamage(const UnitStats &attacker, const UnitStats &defender, const UnitState &defenderState)
{
    DamageResult result = {0, 0, 0, false, false, false};

    if ((rand() % 100) < defender.evasion)
    {
        result.isMiss = true;
        return result;
    }

    float curDef = (float)defender.defense;
    float curRes = (float)defender.resistance;
    if (defenderState.isGuarding)
    {
        curDef *= (1.0f + defender.guardBonus / 100.0f);
        curRes *= (1.0f + defender.guardBonus / 100.0f);
    }

    result.isCrit = (rand() % 100) < attacker.critChance;
    float critMult = result.isCrit ? 1.75f : 1.0f;
    float defMit = 100.0f / (100.0f + curDef);
    int finalPhys = (int)(attacker.physicDmg * critMult * defMit);
    result.finalPhys = finalPhys;

    float resMit = (100.0f - curRes) / 100.0f;
    if (resMit < 0)
        resMit = 0;

    float syncBonus = 1.0f;
    if (defenderState.isAfflicted)
    {
        syncBonus = (100.0f + attacker.synchronizeBonus) / 100.0f;
        result.isSync = true;
    }

    int finalElem = (int)(attacker.elementalDmg * resMit * syncBonus);
    result.finalElem = finalElem;

    result.finalDamage = (std::max)(1, finalPhys + finalElem);
    return result;
}
