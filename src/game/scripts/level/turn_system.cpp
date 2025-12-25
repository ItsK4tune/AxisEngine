#include <game/scripts/level/turn_system.h>
#include <game/scripts/level/vision_system.h>
#include <game/scripts/team/team.h>
#include <iostream>

static void ResetTurn(LevelModel& level)
{
    if (level.team1) level.team1->OnTurnReset();
    if (level.team2) level.team2->OnTurnReset();
}

static void ResetPhase(LevelModel& level)
{
    if (level.team1) level.team1->OnPhaseReset();
    if (level.team2) level.team2->OnPhaseReset();
}

static void ResetCycle(LevelModel& level)
{
    if (level.team1) level.team1->OnCycleReset();
    if (level.team2) level.team2->OnCycleReset();
}

void TurnSystem::SwitchTurn(LevelModel& level, Scene* scene, bool isManualSkip)
{
    if (isManualSkip)
    {
        if (level.prevManualEnd)
        {
            AdvancePhase(level);
            level.prevManualEnd = false;
            level.selectedUnit = entt::null;

            VisionSystem::UpdateFogOfWar(level, scene);
            return;
        }

        level.prevManualEnd = true;
    }
    else
    {
        level.prevManualEnd = false;
    }

    level.activeTeamID = (level.activeTeamID == 1) ? 2 : 1;
    level.selectedUnit = entt::null;

    ResetTurn(level);
    VisionSystem::UpdateFogOfWar(level, scene);
}

void TurnSystem::AdvancePhase(LevelModel& level)
{
    ResetPhase(level);

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

        ResetCycle(level);

        level.movementPhaseFirstTeamIsTeam1 = !level.movementPhaseFirstTeamIsTeam1;
        level.actionPhaseFirstTeamIsTeam1   = !level.actionPhaseFirstTeamIsTeam1;

        level.activeTeamID = 1;
        break;
    }

    std::cout << "[TurnSystem] Phase changed to "
              << static_cast<int>(level.currentPhase) << "\n";
}