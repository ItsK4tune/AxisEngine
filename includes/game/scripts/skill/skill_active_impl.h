#pragma once

#include <game/scripts/skill/skill.h>
#include <game/scripts/unit/unit.h>
#include <game/scripts/team/team.h>
#include <iostream>

class HeavyStrikeSkill : public Skill
{
public:
    HeavyStrikeSkill() : Skill(6, 0, 0, -1, -1) {}

    std::string GetName() const override
    {
        return "HeavyStrike";
    }

    SkillType GetType() const override
    {
        return SkillType::ACTIVE;
    }

    bool CanTrigger(SkillTrigger t, Unit *owner, Unit *target) const override
    {
        return t == SkillTrigger::OnAttack;
    }

    void OnTrigger(SkillTrigger t, Unit *owner, Unit *target) override
    {
        if (t != SkillTrigger::OnAttack || target == nullptr)
            return;

        if (!owner->CanConsumeAP(GetAPCost()).second)
            return;

        owner->ConsumeAP(GetAPCost());

        std::cout << "[HeavyStrikeSkill] Heavy Strike!\n";

        UnitStats clonedStats = owner->stats;
        clonedStats.physicDmg *= 2;

        target->ReceiveDamage(clonedStats);
    }
};