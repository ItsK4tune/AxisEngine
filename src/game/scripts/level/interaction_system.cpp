#include <game/scripts/level/interaction_system.h>
#include <game/scripts/level/vision_system.h>
#include <game/scripts/level/turn_system.h>
#include <game/scripts/team/team.h>
#include <game/scripts/unit/unit.h>
#include <game/commons/utils/hex_astar.h>
#include <game/commons/utils/scene_helper.h>

static std::unordered_set<HexCoord> BuildWalkableSet(Scene *scene, const std::vector<Tile *> &tiles)
{
    std::unordered_set<HexCoord> result;
    for (auto tileEntity : tiles)
    {
        result.insert(tileEntity->gridPos);
    }
    return result;
}

void InteractionSystem::Update(LevelModel &level, Scene *scene, Application *app, float dt)
{
    if (app->GetKeyboard().IsKeyDown(GLFW_KEY_P))
    {
        TurnSystem::SwitchTurn(level, scene, true);
        UpdateUI(level, scene);
        return;
    }

    if (app->GetKeyboard().IsKeyDown(GLFW_KEY_SPACE))
    {
        if (level.currentPhase == GamePhase::ACTION)
        {
            Team *team = level.GetActiveTeam();
            for (auto unitEntity : team->GetUnits())
            {
                if (Unit *unit = SceneHelper::GetScriptInstance<Unit>(scene, unitEntity))
                {
                    unit->state.wantToUseSkill = !unit->state.wantToUseSkill;
                }
            }

            Unit *firstUnit = SceneHelper::GetScriptInstance<Unit>(scene, team->GetUnits()[0]);
            std::cout << "[InteractionSystem] Team " << level.activeTeamID << " set all units to "
                      << (firstUnit && firstUnit->state.wantToUseSkill ? "" : "NOT ") << "use skills.\n";
        }
    }

    if (app->GetMouse().IsLeftMouseClicked())
    {
        HandleClick(level, scene, app);
    }

    UpdateUI(level, scene);
}

void InteractionSystem::HandleClick(LevelModel &level, Scene *scene, Application *app)
{
    glm::vec3 rayFrom, rayTo;
    GetMouseRay(rayFrom, rayTo, scene, app);

    btCollisionWorld::ClosestRayResultCallback rayCallback(
        BulletGLMHelpers::convert(rayFrom),
        BulletGLMHelpers::convert(rayTo));

    app->GetPhysicsWorld().GetWorld()->rayTest(
        BulletGLMHelpers::convert(rayFrom),
        BulletGLMHelpers::convert(rayTo),
        rayCallback);

    if (!rayCallback.hasHit())
        return;

    entt::entity hitEntity = (entt::entity)rayCallback.m_collisionObject->getUserIndex();
    if (!scene->registry.valid(hitEntity))
        return;

    Team *activeTeam = level.GetActiveTeam();

    if (Unit *clickedUnit = SceneHelper::GetScriptInstance<Unit>(scene, hitEntity))
    {
        if (clickedUnit->state.team == level.GetActiveTeam())
        {
            level.selectedUnit = hitEntity;
            std::cout << "[InteractionSystem] Team " << level.activeTeamID << " selected unit at " << clickedUnit->state.gridPos.q << "," << clickedUnit->state.gridPos.r << "," << clickedUnit->state.gridPos.h << std::endl;
            return;
        }

        if (level.currentPhase == GamePhase::ACTION && level.selectedUnit != entt::null)
        {
            Unit *attacker = SceneHelper::GetScriptInstance<Unit>(scene, level.selectedUnit);
            if (!attacker)
                return;

            if (attacker->state.wantToUseSkill)
            {
                if (!attacker->UseActiveSkills(SkillTrigger::OnAttack, clickedUnit))
                {
                    return;
                }
            }
            else
            {
                if(!attacker->Attack(clickedUnit))
                {
                    return;
                }
            }

            if (clickedUnit->stats.currentHP <= 0)
            {
                Team *enemyTeam = (clickedUnit->state.team == level.team1) ? level.team2 : level.team1;
                enemyTeam->RemoveUnit(hitEntity);

                if (enemyTeam->IsDefeated())
                {
                    level.currentPhase = GamePhase::END_GAME;
                    std::cout << ">>> GAME OVER <<<\n";
                }
            }

            UpdateUI(level, scene);
            TurnSystem::SwitchTurn(level, scene, false);
            return;
        }
    }

    if (level.currentPhase == GamePhase::MOVEMENT && level.selectedUnit != entt::null)
    {
        Tile *targetTile = SceneHelper::GetScriptInstance<Tile>(scene, hitEntity);
        if (!targetTile)
            return;

        Unit *unit = SceneHelper::GetScriptInstance<Unit>(scene, level.selectedUnit);
        if (!unit || unit->movement.isMoving)
            return;

        HexCoord start = unit->state.gridPos;
        HexCoord target = targetTile->gridPos;

        std::unordered_set<HexCoord> walkable = BuildWalkableSet(scene, level.tiles);
        walkable.insert(start);

        glm::vec3 worldStart = HexMath::HexToWorld(start);
        glm::vec3 worldTarget = HexMath::HexToWorld(target);

        std::vector<glm::vec3> smoothPath;

        if (!HexAStar::FindSmoothPath(worldStart, worldTarget, walkable, smoothPath))
        {
            std::cout << "[Move] No smooth path found or blocked\n";
            return;
        }

        if (!unit->MoveByWorldPath(smoothPath))
        {
            return;
        }

        UpdateUI(level, scene);
        TurnSystem::SwitchTurn(level, scene, false);
    }
}

void InteractionSystem::UpdateUI(LevelModel &level, Scene *scene)
{
    if (level.uiEntity == entt::null)
        return;

    auto &txt = scene->registry.get<UITextComponent>(level.uiEntity);
    Team *t = level.GetActiveTeam();
    std::string phaseStr, pointsStr;

    switch (level.currentPhase)
    {
    case GamePhase::PLACEMENT:
        phaseStr = "PLACEMENT";
        pointsStr = "Click to Ready";
        break;
    case GamePhase::MOVEMENT:
        phaseStr = "MOVE PHASE";
        pointsStr = "MP: " + std::to_string(t->stats.currentMP);
        break;
    case GamePhase::ACTION:
        phaseStr = "ACTION PHASE";
        pointsStr = "AP: " + std::to_string(t->stats.currentAP);
        break;
    case GamePhase::END_GAME:
        phaseStr = "GAME OVER";
        pointsStr = "";
        break;
    }

    txt.text = phaseStr + " | Team " + std::to_string(level.activeTeamID) + " | " + pointsStr;
}

void InteractionSystem::GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd, Scene *scene, Application *app)
{
    float mx = app->GetMouse().GetLastX();
    float my = app->GetMouse().GetLastY();
    int w = app->GetWidth();
    int h = app->GetHeight();

    glm::vec3 ray_nds((2.0f * mx) / w - 1.0f, 1.0f - (2.0f * my) / h, 1.0f);
    glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.0, 1.0);

    entt::entity camEntity = scene->GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &cam = scene->registry.get<CameraComponent>(camEntity);
    glm::vec4 ray_eye = glm::inverse(cam.projectionMatrix) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    glm::vec3 ray_wor = glm::normalize(glm::vec3(glm::inverse(cam.viewMatrix) * ray_eye));
    auto &camTrans = scene->registry.get<TransformComponent>(camEntity);
    outOrigin = camTrans.position;
    outEnd = outOrigin + ray_wor * 1000.0f;
}