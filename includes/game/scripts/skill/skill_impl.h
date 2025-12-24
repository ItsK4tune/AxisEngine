// #pragma once

// #include <game/scripts/skill.h>
// #include <game/scripts/unit/unit.h>

// #include <iostream>
// #include <algorithm>

// class TankUpSkill : public Skill {
// public:
//     std::string GetName() const override { return "TankUp"; }
//     SkillType GetType() const override { return SkillType::PASSIVE; }
    
//     void OnTurnStart(Unit* owner) override {
//         owner->stats.currentHP = (std::min)((float)owner->stats.maxHP, owner->stats.currentHP + 5.0f);
//         std::cout << "[TankUpSkill] Healed 5 HP.\n";
//     }
// };

// class HeavyStrikeSkill : public Skill {
// public:
//     std::string GetName() const override { return "HeavyStrike"; }
//     SkillType GetType() const override { return SkillType::ACTIVE; }
//     int GetCost() override { return 6; }

//     void Execute(Unit* owner, Unit* target) override {
//         std::cout << "[HeavyStrikeSkill] Heavy Strike!\n";
//         target->TakeDamage(owner->stats.physicDmg * 2, 0, owner);
//     }
// };