#include <states/game_state.h>
#include <scripts/camera_controller.h>

void GameState::OnEnter()
{
    LoadScene("scenes/game.scene");
    SetCursorMode(Input::CursorMode::Normal);

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);
}

void GameState::OnUpdate(float dt)
{

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
