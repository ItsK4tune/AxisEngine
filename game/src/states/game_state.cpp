#include <states/game_state.h>
#include <axis/axis_script.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game.axs");
    SetCursorMode(Input::CursorMode::Normal);

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);
}

void GameState::OnUpdate(float dt)
{
    auto& kb = GetKeyboard();

    static bool pLast = false;
    bool pNow = kb.GetKey(Input::Key::P) || kb.IsKeyDown(Input::Key::P);
    if (pNow && !pLast)
        QueueLoadScene("scenes/game2.axs");
    pLast = pNow;

    static bool oLast = false;
    bool oNow = kb.GetKey(Input::Key::O) || kb.IsKeyDown(Input::Key::O);
    if (oNow && !oLast)
        QueuePopScene();
    oLast = oNow;
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
