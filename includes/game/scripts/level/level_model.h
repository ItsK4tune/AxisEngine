#pragma once

#include <vector>
#include <entt/entt.hpp>
#include <game/commons/enums/game_phase.h>
#include <game/scripts/tile/tile.h>

class Team;

struct LevelModel
{
    Team *team1 = nullptr;
    Team *team2 = nullptr;

    GamePhase currentPhase = GamePhase::PLACEMENT;
    int activeTeamID = 1;
    bool prevManualEnd = false;

    std::vector<Tile *> tiles;
    entt::entity selectedUnit = entt::null;
    entt::entity uiEntity = entt::null;

    bool movementPhaseFirstTeamIsTeam1 = true;
    bool actionPhaseFirstTeamIsTeam1 = false;

    Team *GetUnactiveTeam() const
    {
        return (activeTeamID == 1) ? team2 : team1;
    }

    Team *GetActiveTeam() const
    {
        return (activeTeamID == 1) ? team1 : team2;
    }
};