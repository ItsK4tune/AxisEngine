#pragma once

#include <engine/core/scriptable.h>
#include <game/scripts/unit/unit_data.h>
#include <game/scripts/unit/unit_movement.h>
#include <game/scripts/unit/unit_combat.h>
#include <game/scripts/skill/skill.h>

class Unit : public Scriptable
{
public:
    UnitStats stats;
    UnitState state;

    UnitMovement movement;
    UnitCombat combat;
    std::vector<std::shared_ptr<Skill>> skills;

    void OnCreate() override;
    void OnUpdate(float dt) override;

    Team *GetTeam() const;
    std::pair<bool, bool> CanConsumeAP(int cost) const;
    bool ConsumeMP(int cost);
    bool ConsumeAP(int cost);

    void InitFromFile(const std::string &path);

    void MoveByHexPath(const std::vector<HexCoord> &path);
    void MoveByWorldPath(const std::vector<glm::vec3> &path);
    void Attack(Unit *target);
    void Guard();
    void ResetState();
    void ReceiveDamage(const UnitStats &attackerStats);
    void UsePassiveSkills(SkillTrigger trigger, Unit *target = nullptr);
    void UseActiveSkills(SkillTrigger trigger, Unit *target = nullptr);

private:
    void Die();
};