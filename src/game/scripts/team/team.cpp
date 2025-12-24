#include <game/scripts/team/team.h>
#include <game/scripts/unit/unit.h>
#include <engine/ecs/component.h>
#include <iostream>

void Team::OnCreate()
{
    m_Resources.ResetCycle(stats);
}

void Team::ResetCycle()
{
    m_Resources.ResetCycle(stats);

    m_Roster.CleanUp(m_Scene);

    for (auto e : GetUnits())
    {
        if (m_Scene->registry.all_of<ScriptComponent>(e))
        {
            auto u = GetScript<Unit>(e);
            if (u)
            {
                u->ResetState();
            }
        }
    }
}

bool Team::IsDefeated()
{
    return !m_Roster.HasAliveUnits(m_Scene);
}