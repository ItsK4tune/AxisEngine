#include <game/scripts/level_manager.h>
#include <game/scripts/team.h>
#include <game/scripts/unit.h>
#include <game/scripts/tile.h>
#include <game/commons/utils/hex_astar.h>

#include <engine/utils/bullet_glm_helpers.h>
#include <engine/ecs/component.h>
#include <engine/utils/filesystem.h>

#include <fstream>
#include <iostream>

static std::unordered_set<HexCoord> BuildWalkableSet(Scene *scene, const std::vector<entt::entity> &tiles)
{
    std::unordered_set<HexCoord> result;
    result.reserve(tiles.size());

    for (auto tileEntity : tiles)
    {
        if (!scene->registry.valid(tileEntity))
            continue;

        if (scene->registry.all_of<ScriptComponent>(tileEntity))
        {
            auto &nsc = scene->registry.get<ScriptComponent>(tileEntity);
            Tile *tileScript = static_cast<Tile *>(nsc.instance);
            result.insert(tileScript->gridPos);
        }
    }

    return result;
}

void LevelManager::OnCreate()
{
    CreateTeamScripts();
    LoadLevelFile("resources/levels/test.lvl");

    SpawnUnit(0, 0, 0, 1);
    SpawnUnit(0, 1, 0, 2);

    auto view = m_Scene->registry.view<UITextComponent>();
    for (auto e : view)
    {
        m_UIEntity = e;
        break;
    }

    currentPhase = GamePhase::PLACEMENT;
    activeTeamID = 1;
    prevManualEnd = false;

    std::cout << "[LevelManager] game phase: PLACEMENT\n";
    UpdateFogOfWar();
    UpdateUIText();
}

void LevelManager::OnUpdate(float dt)
{
    switch (currentPhase)
    {
    case GamePhase::PLACEMENT:
        UpdatePlacement();
        break;
    case GamePhase::MOVEMENT:
        UpdateMovement();
        break;
    case GamePhase::ACTION:
        UpdateAction();
        break;
    case GamePhase::END_GAME:
        break;
    }
}

void LevelManager::UpdatePlacement()
{
    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_P))
    {
        std::cout << "[Placement] Team " << activeTeamID << " finished placement\n";

        if (activeTeamID == 1)
            activeTeamID = 2;
        else
        {
            currentPhase = GamePhase::MOVEMENT;
            activeTeamID = 1;
            m_Team1->ResetState();
            m_Team2->ResetState();
            std::cout << "[LevelManager] game phase: MOVEMENT\n";
        }

        UpdateUIText();
    }
}

void LevelManager::UpdateMovement()
{
    if (m_App->GetMouse().IsLeftMouseClicked())
        HandleLogic();
    EndTurnInput();
}

void LevelManager::UpdateAction()
{
    if (m_SelectedUnit != entt::null && m_App->GetKeyboard().GetKey(GLFW_KEY_G))
    {
        Unit *u = GetScript<Unit>(m_SelectedUnit);
        if (u && u->teamID == activeTeamID)
        {
            if (GetActiveTeamScript()->ConsumeActionPoints(u->stats.actionCost))
            {
                u->Guard();
                SwitchTurn(currentPhase, false);
                return;
            }

            std::cout << "[LevelManager] not enough AP\n";
        }
    }

    if (m_App->GetMouse().IsLeftMouseClicked())
        HandleLogic();
    EndTurnInput();
}

void LevelManager::UpdateFogOfWar()
{
    std::cout << "[LevelManager] Update fog of war" << std::endl;
    for (auto tileEntity : tiles)
    {
        if (!m_Scene->registry.valid(tileEntity))
            continue;

        if (auto *tileScript = GetScript<Tile>(tileEntity))
        {
            tileScript->SetVisibility(TileVisibility::FOGGED);
        }
    }

    Team *currentTeam = GetActiveTeamScript();
    const auto &myUnits = currentTeam->GetUnits();

    for (auto unitEntity : myUnits)
    {
        Unit *u = GetScript<Unit>(unitEntity);
        if (!u || u->stats.currentHP <= 0)
            continue;

        for (auto tileEntity : tiles)
        {
            if (auto *tileScript = GetScript<Tile>(tileEntity))
            {
                int dist = HexMath::Distance(u->gridPos, tileScript->gridPos);

                if (dist <= u->stats.lightRadius)
                {
                    tileScript->SetVisibility(TileVisibility::VISIBLE);
                }
            }
        }

        if (m_Scene->registry.all_of<MeshRendererComponent>(unitEntity))
        {
            auto &render = m_Scene->registry.get<MeshRendererComponent>(unitEntity);
            render.visible = true;
        }
    }

    int enemyTeamID = (activeTeamID == 1) ? 2 : 1;
    Team *enemyTeam = (activeTeamID == 1) ? m_Team2 : m_Team1;

    for (auto enemyEntity : enemyTeam->GetUnits())
    {
        Unit *enemyUnit = GetScript<Unit>(enemyEntity);
        if (!enemyUnit)
            continue;

        bool isVisible = false;

        for (auto tileEntity : tiles)
        {
            if (auto *tileScript = GetScript<Tile>(tileEntity))
            {
                if (tileScript->gridPos == enemyUnit->gridPos)
                {
                    if (tileScript->visibility == TileVisibility::VISIBLE)
                    {
                        isVisible = true;
                    }
                    break;
                }
            }
        }

        if (m_Scene->registry.all_of<MeshRendererComponent>(enemyEntity))
        {
            auto &render = m_Scene->registry.get<MeshRendererComponent>(enemyEntity);
            render.visible = isVisible;
        }
    }
}

void LevelManager::SwitchTurn(GamePhase phase, bool isManual)
{
    if (prevManualEnd && isManual)
    {
        switch (phase)
        {
        case GamePhase::MOVEMENT:
            currentPhase = GamePhase::ACTION;
            activeTeamID = 2;
            prevManualEnd = false;
            std::cout << "[LevelManager] game phase: ACTION\n";
            break;
        case GamePhase::ACTION:
            currentPhase = GamePhase::PLACEMENT;
            activeTeamID = 1;
            prevManualEnd = false;
            m_Team1->ResetState();
            m_Team2->ResetState();
            std::cout << "[LevelManager] game phase: PLACEMENT\n";
            break;
        default:
            break;
        }
    }
    else
    {
        std::cout << "[LevelManager] change order\n";
        prevManualEnd = isManual;
        activeTeamID = (activeTeamID == 1) ? 2 : 1;
    }

    m_SelectedUnit = entt::null;
    UpdateFogOfWar();
    UpdateUIText();
}

void LevelManager::EndTurnInput()
{
    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_P))
    {
        std::cout << "[LevelManager] Team " << activeTeamID << " ended turn\n";
        SwitchTurn(currentPhase, true);
    }
}

void LevelManager::HandleLogic()
{
    glm::vec3 rayFrom, rayTo;
    GetMouseRay(rayFrom, rayTo);

    btCollisionWorld::ClosestRayResultCallback rayCallback(
        BulletGLMHelpers::convert(rayFrom),
        BulletGLMHelpers::convert(rayTo));

    m_App->GetPhysicsWorld().GetWorld()->rayTest(
        BulletGLMHelpers::convert(rayFrom),
        BulletGLMHelpers::convert(rayTo),
        rayCallback);

    if (!rayCallback.hasHit())
        return;

    entt::entity hit = (entt::entity)rayCallback.m_collisionObject->getUserIndex();
    if (!m_Scene->registry.valid(hit))
        return;

    Team *myTeam = GetActiveTeamScript();

    if (Unit *clicked = GetScript<Unit>(hit))
    {
        if (clicked->teamID == activeTeamID)
        {
            m_SelectedUnit = hit;
            std::cout << "[Select] Team " << activeTeamID << " selected unit\n";
            return;
        }

        if (currentPhase != GamePhase::ACTION || m_SelectedUnit == entt::null)
            return;

        Unit *attacker = GetScript<Unit>(m_SelectedUnit);
        if (!attacker)
            return;

        int dist = HexMath::Distance(attacker->gridPos, clicked->gridPos);
        if (dist > attacker->stats.attackRange)
        {
            std::cout << "[Action] Out of range\n";
            return;
        }

        if (!myTeam->ConsumeActionPoints(attacker->stats.actionCost))
        {
            std::cout << "[Action] Not enough AP\n";
            return;
        }

        attacker->Attack(clicked);
        if (clicked->stats.currentHP <= 0)
            (clicked->teamID == 1 ? m_Team1 : m_Team2)->RemoveUnit(hit);

        CheckWinCondition();
        UpdateUIText();
        SwitchTurn(currentPhase, false);
        return;
    }

    if (currentPhase != GamePhase::MOVEMENT || m_SelectedUnit == entt::null)
        return;

    Tile *tile = GetScript<Tile>(hit);
    if (!tile)
        return;

    Unit *unit = GetScript<Unit>(m_SelectedUnit);
    if (!unit || unit->isMoving)
        return;

    if (!myTeam->ConsumeMovePoints(unit->stats.moveCost))
    {
        std::cout << "[Move] Not enough MP\n";
        return;
    }

    HexCoord start = unit->gridPos;
    HexCoord target = tile->gridPos;

    int dist = HexMath::Distance(start, target);
    if (dist > unit->stats.moveRadius)
    {
        std::cout << "[Move] Target too far: " << dist
                  << " tiles, max allowed: " << unit->stats.moveRadius << "\n";
        myTeam->ConsumeMovePoints(-unit->stats.moveCost);
        return;
    }

    std::vector<HexCoord> path;
    std::unordered_set<HexCoord> walkable = BuildWalkableSet(m_Scene, tiles);
    if (!HexAStar::FindPath(start, target, walkable, path))
    {
        std::cout << "[Move] No path found\n";
        myTeam->ConsumeMovePoints(-unit->stats.moveCost);
        return;
    }

    unit->gridPos = target;
    unit->MoveByPath(path);

    unit->gridPos = path.back();
    unit->MoveByPath(path);

    UpdateUIText();
    SwitchTurn(currentPhase, false);
}

void LevelManager::UpdateUIText()
{
    if (m_UIEntity == entt::null || !m_Scene->registry.valid(m_UIEntity))
        return;

    auto &txt = m_Scene->registry.get<UITextComponent>(m_UIEntity);
    Team *t = GetActiveTeamScript();
    std::string phaseStr, pointsStr;

    switch (currentPhase)
    {
    case GamePhase::PLACEMENT:
        phaseStr = "PLACEMENT";
        pointsStr = "Click to Ready";
        break;
    case GamePhase::MOVEMENT:
        phaseStr = "MOVE PHASE";
        pointsStr = "MP: " + std::to_string(t->currentMovePoints);
        break;
    case GamePhase::ACTION:
        phaseStr = "ACTION PHASE";
        pointsStr = "AP: " + std::to_string(t->currentActionPoints);
        break;
    case GamePhase::END_GAME:
        phaseStr = "GAME OVER";
        pointsStr = "";
        break;
    }

    txt.text = phaseStr + " | Team " + std::to_string(activeTeamID) + " | " + pointsStr;
}

void LevelManager::GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd)
{
    float mx = m_App->GetMouse().GetLastX();
    float my = m_App->GetMouse().GetLastY();
    int w = m_App->GetWidth();
    int h = m_App->GetHeight();

    glm::vec3 ray_nds((2.0f * mx) / w - 1.0f, 1.0f - (2.0f * my) / h, 1.0f);
    glm::vec4 ray_clip(ray_nds.x, ray_nds.y, -1.0, 1.0);

    entt::entity camEntity = m_Scene->GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &cam = m_Scene->registry.get<CameraComponent>(camEntity);
    glm::vec4 ray_eye = glm::inverse(cam.projectionMatrix) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    glm::vec3 ray_wor = glm::normalize(glm::vec3(glm::inverse(cam.viewMatrix) * ray_eye));
    auto &camTrans = m_Scene->registry.get<TransformComponent>(camEntity);
    outOrigin = camTrans.position;
    outEnd = outOrigin + ray_wor * 1000.0f;
}

void LevelManager::CheckWinCondition()
{
    if (m_Team1->IsDefeated())
        currentPhase = GamePhase::END_GAME;
    else if (m_Team2->IsDefeated())
        currentPhase = GamePhase::END_GAME;
}

Team *LevelManager::GetActiveTeamScript()
{
    return (activeTeamID == 1) ? m_Team1 : m_Team2;
}

void LevelManager::CreateTeamScripts()
{
    auto createTeam = [&](int id, const std::string &name) -> Team *
    {
        auto e = m_Scene->createEntity();
        m_Scene->registry.emplace<InfoComponent>(e, name, "team_manager");
        auto &sc = m_Scene->registry.emplace<ScriptComponent>(e);
        sc.Bind<Team>();
        sc.instance = sc.InstantiateScript();
        sc.instance->Init(e, m_Scene, m_App);
        sc.instance->OnCreate();
        Team *t = static_cast<Team *>(sc.instance);
        t->teamID = id;
        return t;
    };

    m_Team1 = createTeam(1, "team1");
    m_Team2 = createTeam(2, "team2");
}

void LevelManager::SpawnUnit(int q, int r, int h, int team)
{
    auto e = m_Scene->createEntity();
    m_Scene->registry.emplace<InfoComponent>(e, "unit", "unit");

    auto &t = m_Scene->registry.emplace<TransformComponent>(e);
    t.scale = glm::vec3(0.01f);

    auto &ren = m_Scene->registry.emplace<MeshRendererComponent>(e);
    ren.model = m_App->GetResourceManager().GetModel("playerModel");
    ren.shader = m_App->GetResourceManager().GetShader("modelShader");
    ren.castShadow = true;

    auto &sc = m_Scene->registry.emplace<ScriptComponent>(e);
    sc.Bind<Unit>();
    sc.instance = sc.InstantiateScript();
    sc.instance->Init(e, m_Scene, m_App);

    Unit *unit = static_cast<Unit *>(sc.instance);
    unit->gridPos = {q, r, h};
    unit->teamID = team;

    if (team == 1)
        unit->stats.maxHP = 150;
    else
    {
        unit->stats.maxHP = 100;
        unit->stats.attackRange = 2;
    }
    unit->stats.currentHP = unit->stats.maxHP;

    sc.instance->OnCreate();

    auto &rb = m_Scene->registry.emplace<RigidBodyComponent>(e);
    btCapsuleShape *shape = new btCapsuleShape(0.5f, 2.0f);
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));
    rb.body = m_App->GetPhysicsWorld().CreateRigidBody(0.0f, trans, shape);
    rb.body->setUserIndex((int)e);
    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    rb.body->setActivationState(DISABLE_DEACTIVATION);

    if (team == 1)
        m_Team1->AddUnit(e);
    else
        m_Team2->AddUnit(e);
}

void LevelManager::CreateHexTile(int q, int r, int h)
{
    auto e = m_Scene->createEntity();
    m_Scene->registry.emplace<InfoComponent>(e, "tile", "tile");

    auto &t = m_Scene->registry.emplace<TransformComponent>(e);
    t.position = HexMath::HexToWorld({q, r, h});
    t.position.y = (h * 0.5f) - 0.5f;
    t.scale = glm::vec3(0.0003f);

    auto &ren = m_Scene->registry.emplace<MeshRendererComponent>(e);
    ren.model = m_App->GetResourceManager().GetModel("hex_model");
    ren.shader = m_App->GetResourceManager().GetShader("modelShader");

    auto &rb = m_Scene->registry.emplace<RigidBodyComponent>(e);
    btBoxShape *shape = new btBoxShape(btVector3(0.5f, 0.25f, 0.5f));
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));
    rb.body = m_App->GetPhysicsWorld().CreateRigidBody(0.0f, trans, shape);
    rb.body->setUserIndex((int)e);

    auto &sc = m_Scene->registry.emplace<ScriptComponent>(e);
    sc.Bind<Tile>();
    sc.instance = sc.InstantiateScript();
    sc.instance->Init(e, m_Scene, m_App);

    Tile *tileScript = static_cast<Tile *>(sc.instance);
    tileScript->gridPos = {q, r, h};

    sc.instance->OnCreate();

    tiles.push_back(e);
}

void LevelManager::LoadLevelFile(const std::string &path)
{
    std::ifstream file(FileSystem::getPath(path).c_str());

    if (!file.is_open())
        return;

    int w, h, d;
    if (!(file >> w >> h >> d))
        return;

    std::string dummy;
    std::getline(file, dummy);

    for (int layer = 0; layer < d; layer++)
    {
        for (int row = 0; row < h; row++)
        {
            for (int col = 0; col < w; col++)
            {
                while (file.peek() == '#' || file.peek() == '\n' || file.peek() == '\r' || file.peek() == ' ')
                {
                    if (file.peek() == '#')
                    {
                        std::string c;
                        std::getline(file, c);
                    }
                    else
                        file.get();
                }

                int hasTile;
                if (file >> hasTile)
                {
                    if (hasTile)
                    {
                        int q = col - (row - (row & 1)) / 2;
                        int r = row;
                        CreateHexTile(q, r, layer);
                    }
                }
            }
        }
    }
}
