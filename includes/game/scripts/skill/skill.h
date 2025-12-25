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
    Skill(int ap, int mp, int cd, int range = -1, int casttime = -1)
        : m_APCost(ap), m_MPCost(mp), m_Cooldown(cd), m_Range(range), m_Casttime(casttime) {}
    virtual ~Skill() = default;

    virtual std::string GetName() const = 0;
    virtual SkillType GetType() const = 0;
    int GetAPCost() const { return m_APCost; }
    int GetMPCost() const { return m_MPCost; }
    int GetCooldown() const { return m_Cooldown; }
    int GetCasttime() const { return m_Casttime; }
    int GetCurrentCooldown() const { return m_CurrentCooldown; }
    int GetRange() const { return m_Range; }

    void SetAPCost(int cost) { m_APCost = cost; }
    void SetMPCost(int cost) { m_MPCost = cost; }
    void SetCooldown(int cd) { m_Cooldown = cd; }
    void SetCasttime(int ct) { m_Casttime = ct; }
    void SetCurrentCooldown(int cd) { m_CurrentCooldown = cd; }
    void SetRange(int range) { m_Range = range; }

    virtual bool CanTrigger(SkillTrigger t, Unit *owner, Unit *target) const { return false; }
    virtual void OnTrigger(SkillTrigger t, Unit *owner, Unit *target) {}
    void ReduceCooldown()
    {
        if (m_CurrentCooldown > 0)
            m_CurrentCooldown--;
    }
    void ResetCooldown()
    {
        m_CurrentCooldown = m_Cooldown;
    }

private:
    int m_Cooldown = 0;
    int m_Casttime = 0;
    int m_MPCost = 0;
    int m_APCost = 0;
    int m_Range = -1;

    int m_CurrentCooldown = 0;
};