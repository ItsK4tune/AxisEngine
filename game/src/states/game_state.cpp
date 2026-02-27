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

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);

    GetRenderSystem().SetFilterLayerMask(1);
}

void GameState::OnUpdate(float dt)
{
    auto& kb = GetKeyboard();

    static bool pLast = false;
    bool pNow = kb.GetKey(Input::Key::I) || kb.IsKeyDown(Input::Key::I);
    if (pNow && !pLast)
        QueueLoadScene("scenes/game2.axs");
    pLast = pNow;

    static bool oLast = false;
    bool oNow = kb.GetKey(Input::Key::O) || kb.IsKeyDown(Input::Key::O);
    if (oNow && !oLast)
        QueuePopScene();
    oLast = oNow;
    static bool escLast = false;
    bool escNow = kb.GetKey(Input::Key::P) || kb.IsKeyDown(Input::Key::P);
    if (escNow && !escLast)
        m_App->PushState<PauseState>();
    escLast = escNow;

    // --- Animation Examples (zxcvbnm) ---
    auto animView = GetScene().registry.view<AnimationComponent>();
    for (auto entity : animView)
    {
        auto& anim = animView.get<AnimationComponent>(entity);
        if (!anim.animator) continue;

        // Z: Play first animation
        if (kb.IsKeyDown(Input::Key::Z) && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "Z Pressed - Playing animation: " << anim.animations[0];
            anim.animator->PlayAnimation(anim.animations[0]);
        }
        // X: Play second animation
        if (kb.IsKeyDown(Input::Key::X) && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "X Pressed - Playing animation: " << anim.animations[1];
            anim.animator->PlayAnimation(anim.animations[1]);
        }
        // C: Crossfade to second
        if (kb.IsKeyDown(Input::Key::C) && anim.animations.size() >= 2)
        {
            LOGGER_INFO("GameState") << "C Pressed - Crossfading to: " << anim.animations[1];
            anim.animator->CrossFade(anim.animations[1], 0.5f);
        }
        // V: Crossfade to first
        if (kb.IsKeyDown(Input::Key::V) && !anim.animations.empty())
        {
            LOGGER_INFO("GameState") << "V Pressed - Crossfading to: " << anim.animations[0];
            anim.animator->CrossFade(anim.animations[0], 0.5f);
        }

        // B: Speed Down
        if (kb.IsKeyDown(Input::Key::B))
        {
            anim.speed = (std::max)(0.1f, anim.speed - 0.1f);
            anim.animator->SetSpeed(anim.speed);
        }
        // N: Speed Up
        if (kb.IsKeyDown(Input::Key::N))
        {
            anim.speed += 0.1f;
            anim.animator->SetSpeed(anim.speed);
        }
        // M: Reset Speed
        if (kb.IsKeyDown(Input::Key::M))
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
