#pragma once

#include <game/scripts/skill/skill.h>
#include <game/scripts/unit/unit.h>
#include <game/scripts/team/team.h>
#include <iostream>

class HeavyStrikeSkill : public Skill
{
public:
    std::string GetName() const override
    {
        return "HeavyStrike";
    }

    SkillType GetType() const override
    {
        return SkillType::ACTIVE;
    }

    bool CanTrigger(SkillTrigger t) const override
    {
        return t == SkillTrigger::OnAttack;
    }

    void OnTrigger(SkillTrigger t, Unit *owner, Unit *target) override
    {
        if (t != SkillTrigger::OnAttack || target == nullptr)
            return; 

        constexpr int COST = 6;

        if (!owner->CanConsumeAP(COST).second)
            return;

        owner->ConsumeAP(COST);

        std::cout << "[HeavyStrikeSkill] Heavy Strike!\n";

        UnitStats clonedStats = owner->stats;
        clonedStats.physicDmg *= 2;
        
        target->ReceiveDamage(clonedStats);
    }
};