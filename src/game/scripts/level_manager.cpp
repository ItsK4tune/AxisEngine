#include <game/scripts/level_manager.h>

#include <algorithm>
#include <game/scripts/unit.h>
#include <game/utils/hex_math.h>
#include <engine/utils/filesystem.h>

void LevelManager::OnCreate()
{
    LoadLevelFile("resources/levels/test.lvl");

    SpawnUnit(0, 0, 0, 1);
    SpawnUnit(3, 3, 0, 2);

    auto view = m_Scene->registry.view<UITextComponent>();
    for (auto e : view)
    {
        m_UIEntity = e;
        break;
    }
    UpdateUIText();
}

void LevelManager::OnUpdate(float dt)
{
    if (m_App->GetMouse().IsLeftMouseClicked())
    {
        HandleClick();
    }

    if (m_App->GetMouse().IsRightMouseClicked())
    {
        EndTurn();
    }
}

void LevelManager::LoadLevelFile(const std::string &path)
{
    std::ifstream file(FileSystem::getPath(path).c_str());

    if (!file.is_open())
    {
        std::cerr << "[LevelManager] Cannot open level file " << FileSystem::getPath(path).c_str() << std::endl;
        return;
    }

    int w, h, d;
    file >> w >> h >> d;

    for (int layer = 0; layer < d; layer++)
    {
        for (int row = 0; row < h; row++)
        {
            for (int col = 0; col < w; col++)
            {
                int hasTile;
                file >> hasTile;
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

void LevelManager::CreateHexTile(int q, int r, int h)
{
    auto e = m_Scene->createEntity();
    auto &t = m_Scene->registry.emplace<TransformComponent>(e);

    t.position = HexMath::HexToWorld({q, r, h});

    t.scale = glm::vec3(0.0003f);

    auto &ren = m_Scene->registry.emplace<MeshRendererComponent>(e);
    ren.model = m_App->GetResourceManager().GetModel("hex_model");
    ren.shader = m_App->GetResourceManager().GetShader("modelShader");

    auto &rb = m_Scene->registry.emplace<RigidBodyComponent>(e);
    btBoxShape *shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));

    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));

    rb.body = m_App->GetPhysicsWorld().CreateRigidBody(0.0f, trans, shape);
    rb.body->setUserIndex((int)e);

    tiles.push_back(e);
}

void LevelManager::SpawnUnit(int q, int r, int h, int team)
{
    auto e = m_Scene->createEntity();

    auto &t = m_Scene->registry.emplace<TransformComponent>(e);
    t.scale = glm::vec3(0.01f);

    auto &ren = m_Scene->registry.emplace<MeshRendererComponent>(e);
    ren.model = m_App->GetResourceManager().GetModel("playerModel");
    ren.shader = m_App->GetResourceManager().GetShader("modelShader");

    auto &sc = m_Scene->registry.emplace<ScriptComponent>(e);
    sc.Bind<Unit>();

    sc.instance = sc.InstantiateScript();
    sc.instance->Init(e, m_Scene, m_App);
    sc.instance->OnCreate();

    Unit *unit = static_cast<Unit *>(sc.instance);
    unit->gridPos = {q, r, h};
    unit->teamID = team;

    t.position = HexMath::HexToWorld({q, r, h});

    auto &rb = m_Scene->registry.emplace<RigidBodyComponent>(e);
    btCapsuleShape *shape = new btCapsuleShape(0.5f, 2.0f);
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(BulletGLMHelpers::convert(t.position));
    rb.body = m_App->GetPhysicsWorld().CreateRigidBody(0.0f, trans, shape);
    rb.body->setUserIndex((int)e);
    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    rb.body->setActivationState(DISABLE_DEACTIVATION);

    units.push_back(e);
}

void LevelManager::HandleClick()
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

    if (rayCallback.hasHit())
    {
        entt::entity hitEntity = (entt::entity)rayCallback.m_collisionObject->getUserIndex();

        if (!m_Scene->registry.valid(hitEntity))
            return;

        if (auto *clickedUnit = GetScript<Unit>(hitEntity))
        {
            // Kiểm tra đúng lượt không
            if ((m_CurrentTurn == Turn::Player1 && clickedUnit->teamID == 1) ||
                (m_CurrentTurn == Turn::Player2 && clickedUnit->teamID == 2))
            {
                m_SelectedUnit = hitEntity;
                std::cout << "[LevelManager] Selected Unit Team " << clickedUnit->teamID << "\n";
            }
            else
            {
                std::cout << "[LevelManager] Wrong Turn! Cannot select enemy.\n";
            }
            return;
        }

        if (m_SelectedUnit != entt::null)
        {
            auto &tileTrans = m_Scene->registry.get<TransformComponent>(hitEntity);
            HexCoord targetHex = HexMath::WorldToHex(tileTrans.position);

            Unit *unit = GetScript<Unit>(m_SelectedUnit);

            if (m_CurrentSP >= MOVE_COST)
            {
                if (!unit->isMoving)
                {
                    unit->MoveTo(targetHex);
                    m_CurrentSP -= MOVE_COST;
                    std::cout << "[LevelManager] Moved. SP left: " << m_CurrentSP << "\n";
                    UpdateUIText();

                    m_SelectedUnit = entt::null;

                    if (m_CurrentSP <= 0)
                        EndTurn();
                }
            }
            else
            {
                std::cout << "[LevelManager] Not enough SP!\n";
            }
        }
    }
}

void LevelManager::EndTurn()
{
    if (m_CurrentTurn == Turn::Player1)
        m_CurrentTurn = Turn::Player2;
    else
        m_CurrentTurn = Turn::Player1;

    m_CurrentSP = MAX_SP;
    m_SelectedUnit = entt::null;

    std::cout << "[LevelManager] Turn Changed. Current: Player " << (m_CurrentTurn == Turn::Player1 ? "1" : "2") << "\n";
    UpdateUIText();
}

void LevelManager::UpdateUIText()
{
    if (m_UIEntity != entt::null && m_Scene->registry.valid(m_UIEntity))
    {
        auto &txt = m_Scene->registry.get<UITextComponent>(m_UIEntity);
        std::string p = (m_CurrentTurn == Turn::Player1) ? "Player 1" : "Player 2";
        txt.text = p + " Turn (SP: " + std::to_string(m_CurrentSP) + ")";
    }
}

void LevelManager::GetMouseRay(glm::vec3 &outOrigin, glm::vec3 &outEnd)
{
    float mouseX = m_App->GetMouse().GetLastX();
    float mouseY = m_App->GetMouse().GetLastY();
    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    float x = (2.0f * mouseX) / width - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / height;
    float z = 1.0f;
    glm::vec3 ray_nds = glm::vec3(x, y, z);
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    entt::entity camEntity = m_Scene->GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto &cam = m_Scene->registry.get<CameraComponent>(camEntity);
    glm::vec4 ray_eye = glm::inverse(cam.projectionMatrix) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    glm::vec3 ray_wor = glm::vec3(glm::inverse(cam.viewMatrix) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    auto &camTrans = m_Scene->registry.get<TransformComponent>(camEntity);
    outOrigin = camTrans.position;
    outEnd = outOrigin + ray_wor * 1000.0f;
}