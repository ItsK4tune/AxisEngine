#include <game/states/placeholder_state.h>

#include <engine/core/application.h>
#include <game/scripts/camera_controller.h>
#include <iostream>

void PlaceHolderState::OnEnter()
{
    m_App->GetSceneManager().LoadScene("scenes/placeholder2.scene");

    auto camEntity = m_App->GetScene().GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto &script = m_App->GetScene().registry.emplace<ScriptComponent>(camEntity);
        script.Bind<CameraController>();
    }
}

void PlaceHolderState::OnUpdate(float dt)
{
    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_O))
    {
        CursorMode m_CurrentCursorMode = NextCursorMode(m_App->GetMouse().GetCursorMode());

        m_App->GetMouse().SetCursorMode(m_CurrentCursorMode);

        std::cout << "Cursor Mode Changed To: ";
        switch (m_CurrentCursorMode)
        {
        case CursorMode::Normal:
            std::cout << "Normal\n";
            break;
        case CursorMode::Hidden:
            std::cout << "Hidden\n";
            break;
        case CursorMode::Locked:
            std::cout << "Locked\n";
            break;
        case CursorMode::LockedCenter:
            std::cout << "LockedCenter\n";
            break;
        }
    }

    m_App->GetUIInteractSystem().Update(
        m_App->GetScene(),
        dt,
        m_App->GetMouse());

    m_App->GetScriptSystem().Update(m_App->GetScene(), dt, m_App);

    m_App->GetPhysicsWorld().Update(dt);
    m_App->GetPhysicsSystem().Update(m_App->GetScene());

    m_App->GetAnimationSystem().Update(m_App->GetScene(), dt);
}

void PlaceHolderState::OnRender()
{
    m_App->GetRenderSystem().Render(m_App->GetScene());

    m_App->GetUIRenderSystem().Render(
        m_App->GetScene(),
        (float)m_App->GetWidth(),
        (float)m_App->GetHeight());
}

void PlaceHolderState::OnExit()
{
    m_App->GetSceneManager().UnloadScene("scenes/placeholder2.scene");
}

CursorMode PlaceHolderState::NextCursorMode(CursorMode mode)
{
    switch (mode)
    {
    case CursorMode::Normal:
        return CursorMode::Hidden;
    case CursorMode::Hidden:
        return CursorMode::Locked;
    case CursorMode::Locked:
        return CursorMode::LockedCenter;
    case CursorMode::LockedCenter:
        return CursorMode::Normal;
    }
    return CursorMode::Normal;
}