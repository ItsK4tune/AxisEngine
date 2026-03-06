#include <states/game_state.h>
#include <algorithm>
#include <systems/window/io_handler.h>
#include <core/runtime_core.h>
#include <axis/axis_core.h>
#include <axis/axis_ecs.h>
#include <axis/axis_graphics.h>
#include <axis/axis_input.h>
#include <axis/axis_utils.h>
#include <core/state/state_machine.h>
#include <states/pause_state.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game3.axs");
    SetCursorMode(Input::CursorMode::Normal);

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

    if (input.GetActionDown("LoadNextScene"))
        QueueLoadScene("scenes/game2.axs");

    if (input.GetActionDown("ReloadScene"))
        QueuePopScene();
        
    if (input.GetActionDown("Pause"))
        m_Ctx.runtime->GetStateMachine().PushState(std::make_unique<PauseState>());

    if (GetMouse().IsLeftMouseClicked())
    {
        LOGGER_DEBUG("GameState") << "[Raycast] Left mouse clicked.";
        entt::entity camEntity = EntityManager::GetActiveCamera(GetScene());

        if (camEntity != entt::null && EntityManager::HasComponent<CameraComponent>(GetScene(), camEntity) && EntityManager::HasComponent<TransformComponent>(GetScene(), camEntity))
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(GetScene(), camEntity);
            auto& transform = EntityManager::GetComponent<TransformComponent>(GetScene(), camEntity);
            
            Camera cam(transform.position, camComp.worldUp, camComp.yaw, camComp.pitch);
            
            float mouseX = GetMouse().GetLastX();
            float mouseY = GetMouse().GetLastY();
            float screenW = (float)m_Ctx.io->GetMonitorManager().GetWidth();
            float screenH = (float)m_Ctx.io->GetMonitorManager().GetHeight();
            
            if (screenW > 0.0f && screenH > 0.0f)
            {
                float aspect = screenW / screenH;
                glm::mat4 projMat = glm::perspective(glm::radians(camComp.fov), aspect, camComp.nearPlane, camComp.farPlane);
                glm::vec3 rayDir = cam.GetScreenRay(mouseX, mouseY, screenW, screenH, projMat);
                
                if (!glm::any(glm::isnan(transform.position)) && !glm::any(glm::isnan(rayDir)))
                {
                    RayHit hit = m_Ctx.physics->Raycast(transform.position, rayDir, 1000.0f);
                    if (hit.hasHit && EntityManager::IsValid(GetScene(), hit.entity))
                    {
                        m_SelectedEntity = hit.entity;
                        if (auto* info = EntityManager::TryGetComponent<InfoComponent>(GetScene(), hit.entity))
                        {
                            LOGGER_INFO("GameState") << "Selected entity for animation: " << info->name;
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
