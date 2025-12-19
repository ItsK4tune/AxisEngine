#include <game/scripts/team.h>
#include <game/scripts/unit.h>
#include <engine/ecs/component.h>
#include <algorithm>
#include <iostream>

void Team::OnCreate()
{
    currentMovePoints = maxMovePoints;
    currentActionPoints = maxActionPoints;
}

void Team::AddUnit(entt::entity unitEntity)
{
    m_Units.push_back(unitEntity);
}

void Team::RemoveUnit(entt::entity unitEntity)
{
    m_Units.erase(std::remove(m_Units.begin(), m_Units.end(), unitEntity), m_Units.end());
}

const std::vector<entt::entity> &Team::GetUnits() const
{
    return m_Units;
}

void Team::ResetState()
{
    CleanUpUnits();

    currentMovePoints = maxMovePoints;
    currentActionPoints = maxActionPoints;

    std::cout << "[Team " << teamID << "] Resupply: MP=" << currentMovePoints << " AP=" << currentActionPoints << "\n";

    for (auto e : m_Units)
    {
        if (Unit *unit = GetScript<Unit>(e))
        {
            unit->ResetState();
        }
    }
}

bool Team::IsDefeated()
{
    CleanUpUnits();

    for (auto e : m_Units)
    {
        if (Unit *unit = GetScript<Unit>(e))
        {
            if (unit->stats.currentHP > 0)
                return false;
        }
    }

    return true;
}

bool Team::ConsumeMovePoints(int amount)
{
    if (currentMovePoints >= amount)
    {
        currentMovePoints -= amount;
        return true;
    }

    return false;
}

bool Team::ConsumeActionPoints(int amount)
{
    if (currentActionPoints >= amount)
    {
        currentActionPoints -= amount;
        return true;
    }

    return false;
}

void Team::CleanUpUnits()
{
    m_Units.erase(std::remove_if(m_Units.begin(), m_Units.end(),
                                 [this](entt::entity e)
                                 { return !m_Scene->registry.valid(e); }),
                  m_Units.end());
}