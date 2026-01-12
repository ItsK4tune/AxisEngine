#include <game/states/game_state.h>
#include <game/scripts/camera_controller.h>

void GameState::OnEnter()
{
    m_App->GetSceneManager().LoadScene("scenes/game.scene");
    m_App->GetMouse().SetCursorMode(CursorMode::Locked);
}

void GameState::OnUpdate(float dt)
{
    m_App->GetScriptSystem().Update(m_App->GetScene(), dt, m_App);
    m_App->GetPhysicsWorld().Update(dt);
    m_App->GetPhysicsSystem().Update(m_App->GetScene());
    m_App->GetAnimationSystem().Update(m_App->GetScene(), dt);
}

void GameState::OnRender()
{
    m_App->GetSkyboxRenderSystem().Render(m_App->GetScene());
    m_App->GetRenderSystem().Render(m_App->GetScene());
}

void GameState::OnExit()
{
    m_App->GetSceneManager().ClearAllScenes();
}