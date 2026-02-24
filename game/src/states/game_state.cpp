#include <states/game_state.h>
#include <scripts/camera_controller.h>
#include <input/mouse_manager.h>
#include <scene/scene_manager.h>


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
