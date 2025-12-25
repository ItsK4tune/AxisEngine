#pragma once

#include <game/scripts/skill/skill.h>
#include <game/scripts/unit/unit.h>
#include <algorithm>
#include <iostream>

class TankUpSkill : public Skill
{
public:
    TankUpSkill() : Skill(0, 0, 0, -1, -1) {}

    std::string GetName() const override
    {
        return "TankUp";
    }

    SkillType GetType() const override
    {
        return SkillType::PASSIVE;
    }

    bool CanTrigger(SkillTrigger t, Unit *owner, Unit *target) const override
    {
        return t == SkillTrigger::TurnStart;
    }

    void OnTrigger(SkillTrigger t, Unit *owner, Unit * /*target*/) override
    {
        if (t != SkillTrigger::TurnStart)
            return;

        owner->stats.currentHP = (std::min)(static_cast<float>(owner->stats.maxHP),
                                            owner->stats.currentHP + 5.0f);

        std::cout << "[TankUpSkill] Healed 5 HP.\n";
    }
};
