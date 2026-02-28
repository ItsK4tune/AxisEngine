#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis/axis_core.h>
#include <axis/axis_input.h>
#include <axis/axis_ecs.h>
#include <axis/axis_graphics.h>
#include <algorithm>

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
    auto& input = m_App->GetInputManager();

    if (input.GetActionDown("LoadNextScene"))
        QueueLoadScene("scenes/game2.axs");

    if (input.GetActionDown("ReloadScene"))
        QueuePopScene();
        
    if (input.GetActionDown("Pause"))
        m_App->PushState<PauseState>();

    // --- Raycast Example ---
    if (m_App->GetMouse().IsLeftMouseClicked())
    {
        LOGGER_DEBUG("GameState") << "[Raycast] Left mouse clicked.";
        entt::entity cameraEntity = GetScene().GetActiveCamera();
        if (cameraEntity != entt::null && GetScene().registry.all_of<CameraComponent, TransformComponent>(cameraEntity))
        {
            LOGGER_DEBUG("GameState") << "[Raycast] Primary camera found. Entity ID: " << (uint32_t)cameraEntity;
            auto& camComp = GetScene().registry.get<CameraComponent>(cameraEntity);
            auto& transform = GetScene().registry.get<TransformComponent>(cameraEntity);
            glm::vec3 camPos = transform.position;
            
            Camera cam(camPos, camComp.worldUp, camComp.yaw, camComp.pitch);
            
            float mouseX = m_App->GetMouse().GetLastX();
            float mouseY = m_App->GetMouse().GetLastY();
            float screenW = (float)m_App->GetWidth();
            float screenH = (float)m_App->GetHeight();
            
            LOGGER_DEBUG("GameState") << "[Raycast] MouseX: " << mouseX << " MouseY: " << mouseY;
            LOGGER_DEBUG("GameState") << "[Raycast] ScreenW: " << screenW << " ScreenH: " << screenH;
            
            if (screenW > 0.0f && screenH > 0.0f)
            {
                float aspect = screenW / screenH;
                glm::mat4 projMat = glm::perspective(glm::radians(camComp.fov), aspect, camComp.nearPlane, camComp.farPlane);
                LOGGER_DEBUG("GameState") << "[Raycast] Projection matrix obtained.";
                
                glm::vec3 rayDir = cam.GetScreenRay(mouseX, mouseY, screenW, screenH, projMat);
                
                LOGGER_INFO("GameState") << "Raycast: pos=(" << camPos.x << ", " << camPos.y << ", " << camPos.z 
                                         << ") dir=(" << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << ")";
                
                if (glm::any(glm::isnan(camPos)) || glm::any(glm::isnan(rayDir)))
                {
                    LOGGER_WARN("GameState") << "Raycast vectors contain NaN! Aborting raycast.";
                }
                else
                {
                    LOGGER_DEBUG("GameState") << "[Raycast] Calling PhysicsWorld::Raycast...";
                    RayHit hit = m_App->GetPhysicsWorld().Raycast(camPos, rayDir, 1000.0f);
                    LOGGER_DEBUG("GameState") << "[Raycast] PhysicsWorld::Raycast returned. hasHit: " << (hit.hasHit ? "true" : "false");
                    
                    if (hit.hasHit)
                    {
                        LOGGER_DEBUG("GameState") << "[Raycast] Hit Entity ID: " << ((hit.entity == entt::null) ? "null" : std::to_string((uint32_t)hit.entity));
                        if (hit.entity != entt::null)
                        {
                            LOGGER_DEBUG("GameState") << "[Raycast] Checking if entity is valid in registry...";
                            if (GetScene().registry.valid(hit.entity))
                            {
                                LOGGER_DEBUG("GameState") << "[Raycast] Entity is valid. Querying InfoComponent...";
                                if (GetScene().registry.all_of<InfoComponent>(hit.entity))
                                {
                                    auto& info = GetScene().registry.get<InfoComponent>(hit.entity);
                                    LOGGER_DEBUG("GameState") << "[Raycast] Entity Name: " << info.name;
                                    if (info.name == "Woman")
                                    {
                                        LOGGER_INFO("GameState") << "Clicked entity: Woman";
                                    }
                                }
                                else
                                {
                                    LOGGER_DEBUG("GameState") << "[Raycast] Entity does not have an InfoComponent.";
                                }
                            }
                            else
                            {
                                LOGGER_DEBUG("GameState") << "[Raycast] WARNING: Physics returned an invalid entity ID that does NOT exist in the registry!";
                            }
                        }
                    }
                }
            }
        }
        else
        {
            LOGGER_DEBUG("GameState") << "[Raycast] Primary camera is NULL.";
        }
    }

    // --- Animation Examples (zxcvbnm) ---
    auto animView = GetScene().registry.view<AnimationComponent>();
    for (auto entity : animView)
    {
        auto& anim = animView.get<AnimationComponent>(entity);
        if (!anim.animator) continue;

        // Play first animation
        if (input.GetActionDown("AnimPlayPrimary") && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "AnimPlayPrimary - Playing animation: " << anim.animations[0];
            anim.animator->PlayAnimation(anim.animations[0]);
        }
        // Play second animation
        if (input.GetActionDown("AnimPlaySecondary") && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "AnimPlaySecondary - Playing animation: " << anim.animations[1];
            anim.animator->PlayAnimation(anim.animations[1]);
        }
        // Crossfade to second
        if (input.GetActionDown("AnimCrossfadeSec") && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "AnimCrossfadeSec - Crossfading to: " << anim.animations[1];
            anim.animator->CrossFade(anim.animations[1], 0.5f);
        }
        // Crossfade to first
        if (input.GetActionDown("AnimCrossfadePri") && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "AnimCrossfadePri - Crossfading to: " << anim.animations[0];
            anim.animator->CrossFade(anim.animations[0], 0.5f);
        }

        // Speed Down
        if (input.GetActionDown("AnimSpeedDown"))
        {
            anim.speed = (std::max)(0.1f, anim.speed - 0.1f);
            anim.animator->SetSpeed(anim.speed);
        }
        // Speed Up
        if (input.GetActionDown("AnimSpeedUp"))
        {
            anim.speed += 0.1f;
            anim.animator->SetSpeed(anim.speed);
        }
        // Reset Speed
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
    GetSceneManager().ClearAllScenes();
}
