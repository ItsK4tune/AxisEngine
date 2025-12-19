#pragma once
#include <engine/core/scriptable.h>
#include <vector>

class Team : public Scriptable
{
public:
    int teamID = 0;

    int maxMovePoints = 10;
    int maxActionPoints = 10;
    int currentMovePoints = 10;
    int currentActionPoints = 10;

    void OnCreate() override;

    void AddUnit(entt::entity unitEntity);
    void RemoveUnit(entt::entity unitEntity);
    const std::vector<entt::entity> &GetUnits() const;

    void ResetState();
    bool IsDefeated();

    bool ConsumeMovePoints(int amount);
    bool ConsumeActionPoints(int amount);

private:
    std::vector<entt::entity> m_Units;
    void CleanUpUnits();
};