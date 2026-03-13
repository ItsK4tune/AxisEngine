#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis_component.h>
#include <axis_system.h>
#include <axis_util.h>
#include <axis_platform.h>
#include <axis_core.h>
#include <algorithm>

void GameState::OnEnter()
{
    LoadScene("scenes/game3.axs");
    SetCursorMode(CursorMode::Normal);
    LoadInputBindings("resources/configs/binding.axs");

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);

    GetRenderSystem().SetFilterLayerMask(1);

    // Setup PathFollower for both entities
    auto ally = EntityManager::FindByName(GetScene(), "Woman_Ally");
    if (ally != entt::null) GetScene().registry.emplace<PathFollowerComponent>(ally);
    
    auto enemy = EntityManager::FindByName(GetScene(), "Woman_Enemy");
    if (enemy != entt::null) GetScene().registry.emplace<PathFollowerComponent>(enemy);

    // --- Terrain Setup ---
    auto terrainEntity = EntityManager::CreateEntity(GetScene(), "DemoTerrain");
    auto& info = GetScene().registry.emplace<InfoComponent>(terrainEntity);
    info.name = "DemoTerrain";
    info.tag = "Walkable";

    auto& terrain = GetScene().registry.emplace<TerrainComponent>(terrainEntity);
    auto& terrainPos = GetScene().registry.emplace<PositionComponent>(terrainEntity);
    terrainPos.value = glm::vec3(-256.0f, -10.0f, -256.0f);

    terrain.terrainSize = glm::vec3(512.0f, 50.0f, 512.0f);
    terrain.resolution = 513; terrain.chunkSize = 33;
    terrain.maxHeight = 50.0f; terrain.textureScale = 20.0f;

    auto& res = GetResourceManager();
    res.LoadShader("terrain", "includes/engine/asset/shaders/terrain.vs", "includes/engine/asset/shaders/terrain.fs");
    res.LoadTexture("heightmap_demo", "resources/textures/heightmap_demo.png", false, true);
    res.LoadTexture("splatmap_demo", "resources/textures/splatmap_demo.png", false);
    res.LoadTexture("grass", "resources/textures/grass.png", false);
    res.LoadTexture("dirt", "resources/textures/dirt.png", false);
    res.LoadTexture("rock", "resources/textures/rock.png", false);
    res.LoadTexture("snow", "resources/textures/snow.png", false);

    auto hm = res.GetTexture("heightmap_demo");
    auto sm = res.GetTexture("splatmap_demo");
    auto g = res.GetTexture("grass");
    auto d = res.GetTexture("dirt");
    auto r = res.GetTexture("rock");
    auto sn = res.GetTexture("snow");
    
    if (hm && sm && g && d && r && sn) {
        terrain.heightMap = hm->id;
        terrain.heightMapName = "heightmap_demo";
        terrain.splatMap = sm->id;
        terrain.diffuseLayers.push_back(g->id);
        terrain.diffuseLayers.push_back(d->id);
        terrain.diffuseLayers.push_back(r->id);
        terrain.diffuseLayers.push_back(sn->id);
        terrain.generatePhysics = true;
        terrain.isWalkable = true;
        terrain.needsRebuild = true;
    }

    // Rebuild NavMesh on start
    auto navEntity = EntityManager::CreateEntity(GetScene(), "NavMesh");
    auto& navMesh = GetScene().registry.emplace<NavMeshComponent>(navEntity);
    navMesh.needsRebuild = true;

    // Configure Navigation System
    auto& navSys = GetNavigationSystem();
    navSys.ClearWalkableTags();
    navSys.AddWalkableTag("Walkable");

    // Demo Custom Logic: Smoothest (Custom weight based on Y delta)
    if (ally != entt::null) {
        navSys.SetCustomCostFunction(GetScene(), ally, [](uint32_t current, uint32_t neighbor, const NavMeshComponent& navMesh) {
            float yDelta = std::abs(navMesh.nodes[current].position.y - navMesh.nodes[neighbor].position.y);
            return 1.0f + (yDelta * 20.0f); // Example: Penalize height changes
        });
    }
}

void GameState::OnUpdate(float dt)
{
    auto& input = GetInputManager();

    if (input.GetActionDown("LoadNextScene")) QueueLoadScene("scenes/game2.axs");
    if (input.GetActionDown("ReloadScene")) QueuePopScene();
    if (input.GetActionDown("Pause")) m_Ctx.runtime->GetStateMachine().PushState(std::make_unique<PauseState>());

    // Selection & Movement Logic (Combined into Mouse Left Click)
    if (input.GetActionDown("Select")) 
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = GetMouse().GetLastX();
            float mouseY = GetMouse().GetLastY();
            float screenW = (float)m_Ctx.io->GetMonitorManager().GetWidth();
            float screenH = (float)m_Ctx.io->GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / (screenW > 0 ? screenW : 1.0f) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / (screenH > 0 ? screenH : 1.0f);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = m_Ctx.physics->Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                auto* info = EntityManager::TryGetComponent<InfoComponent>(GetScene(), hit.entity);
                if (info && (info->tag == "ally" || info->tag == "enemy"))
                {
                    m_SelectedEntity = hit.entity;
                    LOGGER_INFO("GameState") << "Selected: " << info->name;
                }
                else if (m_SelectedEntity != entt::null)
                {
                    // If terrain (or something else) is clicked, move selected entity
                    if (auto* follower = EntityManager::TryGetComponent<PathFollowerComponent>(GetScene(), m_SelectedEntity))
                    {
                        follower->lockXPitch = true;
                        follower->lockZRoll = true;
                        GetNavigationSystem().MoveTo(GetScene(), m_SelectedEntity, hit.hitPoint);
                        LOGGER_INFO("GameState") << "Move Command: target=(" << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z << ") entity=" << (uint32_t)m_SelectedEntity;
                    }
                }
            }
        }
    }

    // --- Shader Port Management ---
    auto demoView = GetScene().registry.view<InfoComponent, MaterialComponent>();
    for (auto entity : demoView)
    {
        auto [info, mat] = demoView.get<InfoComponent, MaterialComponent>(entity);
        
        // Port[0]: Team ID (1=Enemy/Red, 2=Ally/Blue)
        if (info.tag == "enemy") mat.desc.ports.data[0] = 1.0f;
        else if (info.tag == "ally") mat.desc.ports.data[0] = 2.0f;
        else mat.desc.ports.data[0] = 0.0f;

        // Port[1]: Selection Glow (1.0=On)
        mat.desc.ports.data[1] = (entity == m_SelectedEntity) ? 1.0f : 0.0f;
    }
}


void GameState::OnFixedUpdate(float fixedDt)
{
}

void GameState::OnRender()
{
}

void GameState::OnExit()
{
    m_SelectedEntity = entt::null;
    GetSceneManager().ClearAllScenes();
}
