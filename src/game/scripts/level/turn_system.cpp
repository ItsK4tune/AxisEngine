#include <game/scripts/level/turn_system.h>
#include <game/scripts/level/vision_system.h>
#include <game/scripts/team/team.h>
#include <iostream>

void TurnSystem::SwitchTurn(LevelModel &level, Scene* scene, bool isManualSkip)
{
    if (isManualSkip)
    {
        if (level.prevManualEnd)
        {
            AdvancePhase(level);
            ResetUnitsIfNeeded(level);
            level.prevManualEnd = false;
            level.selectedUnit = entt::null;
            return;
        }
        level.prevManualEnd = true;
    }

    level.activeTeamID = (level.activeTeamID == 1) ? 2 : 1;
    VisionSystem::UpdateFogOfWar(level, scene);
    level.selectedUnit = entt::null;
}

void TurnSystem::AdvancePhase(LevelModel &level)
{
    switch (level.currentPhase)
    {
    case GamePhase::PLACEMENT:
        level.currentPhase = GamePhase::MOVEMENT;
        level.activeTeamID = level.movementPhaseFirstTeamIsTeam1 ? 1 : 2;
        break;

    case GamePhase::MOVEMENT:
        level.currentPhase = GamePhase::ACTION;
        level.activeTeamID = level.actionPhaseFirstTeamIsTeam1 ? 1 : 2;
        break;

    case GamePhase::ACTION:
        level.currentPhase = GamePhase::PLACEMENT;
        break;
    }

    std::cout << "[TurnSystem] Phase changed to " << (int)level.currentPhase << "\n";
}

void TurnSystem::ResetUnitsIfNeeded(LevelModel &level)
{
    if (level.currentPhase == GamePhase::PLACEMENT)
    {
        if (level.team1)
            level.team1->ResetCycle();
        if (level.team2)
            level.team2->ResetCycle();

        level.movementPhaseFirstTeamIsTeam1 = !level.movementPhaseFirstTeamIsTeam1;
        level.actionPhaseFirstTeamIsTeam1 = !level.actionPhaseFirstTeamIsTeam1;
    }
}