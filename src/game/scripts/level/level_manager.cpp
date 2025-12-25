#include <game/scripts/level/level_manager.h>
#include <game/scripts/level/level_generator.h>
#include <game/scripts/level/interaction_system.h>
#include <game/scripts/level/turn_system.h>
#include <game/scripts/level/vision_system.h>

void LevelManager::OnCreate()
{
    LevelGenerator::InitTeams(m_Scene, m_App, m_Level);
    LevelGenerator::LoadMap("resources/levels/test.lvl", m_Scene, m_App, m_Level);

    LevelGenerator::SpawnUnit(0, 0, 0, m_Level.team1, m_Scene, m_App, m_Level);
    LevelGenerator::SpawnUnit(0, 1, 0, m_Level.team2, m_Scene, m_App, m_Level);

    auto view = m_Scene->registry.view<UITextComponent>();
    for (auto e : view)
    {
        m_Level.uiEntity = e;
        break;
    }

    m_Level.currentPhase = GamePhase::PLACEMENT;
    VisionSystem::UpdateFogOfWar(m_Level, m_Scene);
    std::cout << "[LevelManager] Initialized.\n";
}

void LevelManager::OnUpdate(float dt)
{
    InteractionSystem::Update(m_Level, m_Scene, m_App, dt);
}