#include <states/game_state.h>
#include <scripts/camera_controller.h>

void GameState::OnEnter()
{
    GetSceneManager().LoadScene("scenes/game.scene");
    m_App->GetMouse().SetCursorMode(CursorMode::Normal); // Enable mouse for UI
}

void GameState::OnUpdate(float dt)
{
    GetScriptSystem().Update(m_App->GetScene(), dt, m_App);
    // Physics moved to FixedUpdate
    GetAnimationSystem().Update(m_App->GetScene(), dt);
    GetAudioSystem().Update(m_App->GetScene(), m_App->GetSoundManager());
    GetParticleSystem().Update(m_App->GetScene(), dt);
    GetVideoSystem().Update(m_App->GetScene(), m_App->GetResourceManager(), dt);
    GetUIInteractSystem().Update(m_App->GetScene(), dt, m_App->GetMouse());
}

void GameState::OnFixedUpdate(float fixedDt)
{
    GetPhysicsSystem().Update(m_App->GetScene(), m_App->GetPhysicsWorld(), fixedDt);
    GetVideoSystem().Update(m_App->GetScene(), m_App->GetResourceManager(), fixedDt);
}

void GameState::OnRender()
{
    // GetSkyboxRenderSystem().Render(m_App->GetScene());
    GetRenderSystem().Render(m_App->GetScene());
    GetParticleSystem().Render(m_App->GetScene(), m_App->GetResourceManager());
    GetUIRenderSystem().Render(m_App->GetScene(), m_App->GetWidth(), m_App->GetHeight());
}

void GameState::OnExit()
{
    GetSceneManager().ClearAllScenes();
}