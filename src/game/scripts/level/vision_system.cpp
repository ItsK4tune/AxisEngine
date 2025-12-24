#include <game/scripts/level/vision_system.h>

#include <game/scripts/tile/tile.h>
#include <game/scripts/unit/unit.h>
#include <game/scripts/team/team.h>
#include <game/commons/utils/hex_math.h>
#include <engine/ecs/component.h>
#include <game/commons/utils/scene_helper.h>

void VisionSystem::UpdateFogOfWar(LevelModel &level, Scene *scene)
{
    for (auto tileEntity : level.tiles)
    {
        tileEntity->SetVisibility(TileVisibility::FOGGED);
    }

    Team *currentTeam = level.GetActiveTeam();
    if (currentTeam)
    {
        for (auto unitEntity : currentTeam->GetUnits())
        {
            Unit *u = SceneHelper::GetScriptInstance<Unit>(scene, unitEntity);
            if (!u || u->stats.currentHP <= 0)
                continue;

            if (scene->registry.all_of<MeshRendererComponent>(unitEntity))
            {
                scene->registry.get<MeshRendererComponent>(unitEntity).visible = true;
            }

            for (auto tileEntity : level.tiles)
            {
                int dist = HexMath::Distance(u->state.gridPos, tileEntity->gridPos);
                if (dist <= u->stats.lightRadius)
                {
                    tileEntity->SetVisibility(TileVisibility::VISIBLE);
                }
            }
        }
    }

    Team *enemyTeam = (level.activeTeamID == 1) ? level.team2 : level.team1;

    if (enemyTeam)
    {
        for (auto enemyEntity : enemyTeam->GetUnits())
        {
            Unit *enemyUnit = SceneHelper::GetScriptInstance<Unit>(scene, enemyEntity);
            if (!enemyUnit)
                continue;

            bool isVisible = false;

            for (auto tileEntity : level.tiles)
            {
                if (tileEntity->gridPos == enemyUnit->state.gridPos)
                {
                    if (tileEntity->visibility == TileVisibility::VISIBLE)
                    {
                        isVisible = true;
                    }
                    break;
                }
            }

            if (scene->registry.all_of<MeshRendererComponent>(enemyEntity))
            {
                scene->registry.get<MeshRendererComponent>(enemyEntity).visible = isVisible;
            }
        }
    }
}