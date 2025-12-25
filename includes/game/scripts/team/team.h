#pragma once

#include <engine/core/scriptable.h>
#include <game/scripts/team/team_roster.h>
#include <game/scripts/team/team_resource_manager.h>

class Team : public Scriptable
{
public:
    TeamStats stats;
    TeamConfig config;

    void OnCreate() override;

    void AddUnit(entt::entity unit) { m_Roster.Add(unit); }
    void RemoveUnit(entt::entity unit) { m_Roster.Remove(unit); }
    const std::vector<entt::entity> &GetUnits() const { return m_Roster.GetAll(); }
    void ClearUnit() { m_Roster.Clear(); }

    void OnCycleReset();
    void OnPhaseReset();
    void OnTurnReset();
    bool IsDefeated();

    std::pair<bool, bool> CanConsume(int amount) const { return m_Resources.CanConsume(stats, amount); }
    bool ConsumeMP(int amount) { return m_Resources.ConsumeMP(stats, amount); }
    bool ConsumeAP(int amount) { return m_Resources.ConsumeAP(stats, amount); }

    void AddStartingUnit(const std::string &path) { config.AddFile(path); }

private:
    TeamRoster m_Roster;
    TeamResourceManager m_Resources;
};