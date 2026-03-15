#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis_component.h>
#include <axis_system.h>
#include <axis_util.h>
#include <axis_platform.h>
#include <algorithm>
#include <ecs/unit/decal_component.h>
#include <ecs/logic/decal_system.h>
#include <axis_core.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game3.axs");
    LoadScene("scenes/ui.axs");
    SetCursorMode(CursorMode::Normal);
    LoadInputBindings("resources/configs/binding.axs");

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);

    GetRenderSystem().SetFilterLayerMask(1);
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
                else
                {
                    // Move command IF an entity is selected
                    if (m_SelectedEntity != entt::null)
                    {
                        if (auto* follower = EntityManager::TryGetComponent<PathFollowerComponent>(GetScene(), m_SelectedEntity))
                        {
                            follower->lockXPitch = true;
                            follower->lockZRoll = true;
                            GetNavigationSystem().MoveTo(GetScene(), m_SelectedEntity, hit.hitPoint);
                            LOGGER_INFO("GameState") << "Move Command: target=(" << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z << ") entity=" << (uint32_t)m_SelectedEntity;
                        }
                    }
                    
                    // ALWAYS spawn decal on click if we hit something (terrain, etc)
                    auto* decalSys = m_Ctx.systems->GetSystem<DecalSystem>();
                    if (decalSys)
                    {
                        static uint32_t crackTex = 0;
                        if (crackTex == 0) crackTex = decalSys->LoadDecalTexture("resources/textures/decal_crack.png");
                        
                        auto decalEnt = EntityManager::CreateEntity(GetScene(), "Decal_Crack");
                        auto& dComp = EntityManager::AddComponent<DecalComponent>(GetScene(), decalEnt);
                        dComp.albedoMap = crackTex;
                        dComp.lifetime = 10.0f; // Fade out after 10s
                        dComp.targetTags = {"terrain", "wall"}; // API demo: stick to terrain and wall
                        
                        auto& dPos = EntityManager::AddComponent<PositionComponent>(GetScene(), decalEnt);
                        dPos.value = hit.hitPoint;
                        
                        auto& dRot = EntityManager::AddComponent<RotationComponent>(GetScene(), decalEnt);
                        // Align decal box to surface normal
                        glm::vec3 up = glm::vec3(0, 1, 0);
                        if (glm::abs(glm::dot(hit.hitNormal, up)) > 0.99f) up = glm::vec3(1, 0, 0);
                        dRot.value = glm::quatLookAt(-hit.hitNormal, up); // Decal projects along -Z
                        
                        auto& dScale = EntityManager::AddComponent<ScaleComponent>(GetScene(), decalEnt);
                        dScale.value = glm::vec3(1.0f, 1.0f, 1.0f);
                        
                        LOGGER_INFO("GameState") << "Spawned Decal at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
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
