#include <states/game_state.h>
#include <scripts/camera_controller.h>

void GameState::OnEnter()
{
    GetSceneManager().LoadScene("scenes/game.scene");
    m_App->GetMouse().SetCursorMode(CursorMode::Locked); // AppHandler wrapper not in State yet?
}

void GameState::OnUpdate(float dt)
{
    GetScriptSystem().Update(m_App->GetScene(), dt, m_App);
    // Physics moved to FixedUpdate
    GetAnimationSystem().Update(m_App->GetScene(), dt);
    GetAudioSystem().Update(m_App->GetScene(), m_App->GetSoundManager());
    GetParticleSystem().Update(m_App->GetScene(), dt);
}

void GameState::OnFixedUpdate(float fixedDt)
{
    GetPhysicsSystem().Update(m_App->GetScene(), m_App->GetPhysicsWorld(), fixedDt);
}

void GameState::OnRender()
{
    // GetSkyboxRenderSystem().Render(m_App->GetScene());
    GetRenderSystem().Render(m_App->GetScene());
    GetParticleSystem().Render(m_App->GetScene(), m_App->GetResourceManager());
}

void GameState::OnExit()
{
    GetSceneManager().ClearAllScenes();
}