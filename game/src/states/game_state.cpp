#include <states/game_state.h>
#include <scripts/camera_controller.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game.scene");
    SetCursorMode(CursorMode::Normal);

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);
}

void GameState::OnUpdate(float dt)
{
    // Minimal logic here. Systems are updated by Application.
}

void GameState::OnFixedUpdate(float fixedDt)
{
    // Physics handled by Application Loop
}

void GameState::OnRender()
{
    // Render handled by Application Loop
}

void GameState::OnExit()
{
    GetSceneManager().ClearAllScenes();
}