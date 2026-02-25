#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis/axis_core.h>
#include <axis/axis_input.h>
#include <axis/axis_ecs.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game.axs");
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
