#pragma once

#include <engine/core/scriptable.h>
#include <engine/core/application.h>
#include <vector>

class LevelManager : public Scriptable
{
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    std::vector<entt::entity> tiles;
    
    entt::entity m_SelectedUnit = entt::null; 

    void GenerateGrid(int radius);
    void CreateHexTile(int q, int r);
    void SpawnUnit(int q, int r, bool isPlayer);
    void HandleClick();
    void GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd);
};