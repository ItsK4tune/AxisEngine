#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis/axis_core.h>
#include <axis/axis_input.h>
#include <axis/axis_ecs.h>
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
