#pragma once
#include <string>

class Unit;

enum class SkillType
{
    PASSIVE,
    ACTIVE
};

enum class SkillTrigger
{
    TurnStart,
    TurnEnd,
    OnAttack,
    OnHit,
    OnDamaged,
    OnKill,
    OnMove,
};

class Skill
{
public:
    virtual ~Skill() = default;
    virtual std::string GetName() const = 0;
    virtual SkillType GetType() const = 0;

    virtual bool CanTrigger(SkillTrigger t) const { return false; }
    virtual void OnTrigger(SkillTrigger t, Unit *owner, Unit *target) {}
};