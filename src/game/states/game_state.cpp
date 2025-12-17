#include <game/states/game_state.h>

#include <iostream>

#include <game/scripts/unit.h>
#include <game/scripts/level_manager.h>
#include <game/scripts/camera_controller.h>

void TacticsState::OnEnter()
{
    m_App->GetSceneManager().RegisterScript<CameraController>("CameraController");
    m_App->GetSceneManager().RegisterScript<LevelManager>("LevelManager");

    m_App->GetSceneManager().LoadScene("scenes/game.scene");

    m_App->GetMouse().SetCursorMode(CursorMode::Normal);

    std::cout << "[TacticsState] Level Loaded.\n";
}

void TacticsState::OnUpdate(float dt)
{
    // if (m_App->GetKeyboard().GetKey(GLFW_KEY_ESCAPE)) {

    // }

    m_App->GetScriptSystem().Update(m_App->GetScene(), dt, m_App);
    m_App->GetPhysicsWorld().Update(dt);
    m_App->GetPhysicsSystem().Update(m_App->GetScene());
    m_App->GetAnimationSystem().Update(m_App->GetScene(), dt);
}

void TacticsState::OnRender()
{
    m_App->GetRenderSystem().Render(m_App->GetScene());
    m_App->GetUIRenderSystem().Render(
        m_App->GetScene(),
        (float)m_App->GetWidth(),
        (float)m_App->GetHeight());
}

void TacticsState::OnExit()
{
    m_App->GetSceneManager().ClearAllScenes();
}