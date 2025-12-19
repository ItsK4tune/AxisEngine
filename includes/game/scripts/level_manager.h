#pragma once

#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <vector>
#include <unordered_set>
#include <game/commons/utils/hex_math.h>
#include <game/commons/enums/game_phase.h>

class Team;

class LevelManager : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    GamePhase currentPhase = GamePhase::PLACEMENT;

    Team* m_Team1 = nullptr;
    Team* m_Team2 = nullptr;

    int activeTeamID = 1;
    bool prevManualEnd = false;

    entt::entity m_SelectedUnit = entt::null;
    entt::entity m_UIEntity = entt::null;
    std::unordered_set<HexCoord> m_WalkableTiles;

    void UpdatePlacement();
    void UpdateMovement();
    void UpdateAction();

    void SwitchTurn(GamePhase phase, bool isManual);
    void EndTurnInput();
    void HandleLogic();
    void GetMouseRay(glm::vec3& origin, glm::vec3& end);
    void UpdateUIText();
    void CheckWinCondition();

    Team* GetActiveTeamScript();
    void CreateTeamScripts();
    void SpawnUnit(int q, int r, int h, int team);
    void CreateHexTile(int q, int r, int h);
    void LoadLevelFile(const std::string& path);
};
