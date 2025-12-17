#pragma once
#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <vector>
#include <game/scripts/unit.h>
#include <game/utils/hex_math.h>

enum class Turn
{
    Player1,
    Player2
};

class LevelManager : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

    void UpdateUIText();

private:
    std::vector<entt::entity> tiles;
    std::vector<entt::entity> units;

    entt::entity m_SelectedUnit = entt::null;

    Turn m_CurrentTurn = Turn::Player1;
    int m_CurrentSP = 10;
    const int MAX_SP = 10;
    const int MOVE_COST = 2;

    entt::entity m_UIEntity = entt::null;

    void LoadLevelFile(const std::string &path);
    void CreateHexTile(int q, int r, int h);
    void SpawnUnit(int q, int r, int h, int team);

    void HandleClick();
    void GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd);

    void EndTurn();
};