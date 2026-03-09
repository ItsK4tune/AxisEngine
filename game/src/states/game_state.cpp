#include <states/game_state.h>
#include <algorithm>
#include <platform/logic/io_handler.h>
#include <core/logic/engine_core.h>
#include <core/logic/logger.h>
#include <core/logic/state_management.h>
#include <ecs/manager/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <platform/logic/input_system.h>
#include <render/type/graphics_types.h>
#include <states/pause_state.h>
#include <scene/logic/scene_manager.h>
#include <resource/manager/resource_manager.h>
#include <ecs/logic/render_system.h>
#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>

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

    // --- NavMesh Demo Setup ---
    // Navigation & Pathfinding Setup
    auto navEntity = EntityManager::CreateEntity(GetScene(), "NavMesh");
    auto& navMesh = GetScene().registry.emplace<NavMeshComponent>(navEntity);
    navMesh.needsRebuild = true;

    auto womanEnt = EntityManager::FindByName(GetScene(), "Woman");
    if (womanEnt != entt::null) {
        GetScene().registry.emplace<PathFollowerComponent>(womanEnt);
    }
}

void GameState::OnUpdate(float dt)
{
    auto& input = GetInputManager();

    if (input.GetActionDown("LoadNextScene"))
        QueueLoadScene("scenes/game2.axs");

    if (input.GetActionDown("ReloadScene"))
        QueuePopScene();
        
    if (input.GetActionDown("Pause"))
        m_Ctx.runtime->GetStateMachine().PushState(std::make_unique<PauseState>());

    bool selectPressed = input.GetActionDown("Select");
    bool movePressed = input.GetActionDown("MoveTo");

    if (selectPressed || movePressed)
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(GetScene());
        if (camEntity != entt::null && EntityManager::HasComponent<CameraComponent>(GetScene(), camEntity))
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(GetScene(), camEntity)) ? EntityManager::GetComponent<PositionComponent>(GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = GetMouse().GetLastX();
            float mouseY = GetMouse().GetLastY();
            float screenW = (float)m_Ctx.io->GetMonitorManager().GetWidth();
            float screenH = (float)m_Ctx.io->GetMonitorManager().GetHeight();
            
            if (screenW > 0.0f && screenH > 0.0f)
            {
                float x = (2.0f * mouseX) / screenW - 1.0f;
                float y = 1.0f - (2.0f * mouseY) / screenH;
                
                glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
                glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
                rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
                glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
                
                if (!glm::any(glm::isnan(camPos)) && !glm::any(glm::isnan(rayDir)))
                {
                    RayHit hit = m_Ctx.physics->Raycast(camPos, rayDir, 1000.0f);
                    if (hit.hasHit)
                    {
                        if (movePressed) 
                        {
                             LOGGER_DEBUG("GameState") << "MoveTo triggered at: " << hit.hitPoint.x << ", " << hit.hitPoint.y << ", " << hit.hitPoint.z;
                             auto followerView = GetScene().registry.view<PathFollowerComponent>();
                             for (auto ent : followerView) {
                                 auto& follower = followerView.get<PathFollowerComponent>(ent);
                                 follower.targetPosition = hit.hitPoint;
                                 follower.pathPending = true;
                             }
                        }
                        else if (selectPressed && EntityManager::IsValid(GetScene(), hit.entity))
                        {
                            m_SelectedEntity = hit.entity;
                            if (auto* info = EntityManager::TryGetComponent<InfoComponent>(GetScene(), hit.entity))
                            {
                                LOGGER_INFO("GameState") << "Selected entity: " << info->name;
                            }
                        }
                    }
                }
            }
        }
    }

    auto animView = GetScene().registry.view<AnimationComponent>();
    for (auto entity : animView)
    {
        if (entity != m_SelectedEntity) continue;

        auto& anim = animView.get<AnimationComponent>(entity);
        if (!anim.animator) continue;

        if (input.GetActionDown("AnimPlayPrimary") && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "AnimPlayPrimary - Playing animation: " << anim.animations[0];
            anim.animator->PlayAnimation(anim.animations[0]);
        }
        if (input.GetActionDown("AnimPlaySecondary") && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "AnimPlaySecondary - Playing animation: " << anim.animations[1];
            anim.animator->PlayAnimation(anim.animations[1]);
        }
        if (input.GetActionDown("AnimCrossfadeSec") && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "AnimCrossfadeSec - Crossfading to: " << anim.animations[1];
            anim.animator->CrossFade(anim.animations[1], 0.5f);
        }
        if (input.GetActionDown("AnimCrossfadePri") && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "AnimCrossfadePri - Crossfading to: " << anim.animations[0];
            anim.animator->CrossFade(anim.animations[0], 0.5f);
        }

        if (input.GetActionDown("AnimSpeedDown"))
        {
            anim.speed = (std::max)(0.1f, anim.speed - 0.1f);
            anim.animator->SetSpeed(anim.speed);
        }
        if (input.GetActionDown("AnimSpeedUp"))
        {
            anim.speed += 0.1f;
            anim.animator->SetSpeed(anim.speed);
        }
        if (input.GetActionDown("AnimSpeedReset"))
        {
            anim.speed = 1.0f;
            anim.animator->SetSpeed(anim.speed);
        }
    }

    // --- Demo: Shader Port Logic ---
    auto demoView = GetScene().registry.view<InfoComponent, MaterialComponent>();
    for (auto entity : demoView)
    {
        auto [info, mat] = demoView.get<InfoComponent, MaterialComponent>(entity);
        
        if (info.tag == "enemy")
        {
            mat.desc.ports.data[0] = 1.0f; // 1.0 = Enemy (Red in shader)
        }
        else if (info.tag == "ally")
        {
            mat.desc.ports.data[0] = 2.0f; // 2.0 = Ally (Green in shader)
        }
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
