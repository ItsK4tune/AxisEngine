#pragma once

#include <engine/core/state.h>
#include <engine/core/application.h>
#include <iostream>

class PlaceHolderState : public State
{
public:
    void OnEnter() override
    {
        m_App->GetSceneManager().LoadScene("scenes/placeholder.scene");

        auto view = m_App->GetScene().registry.view<UITransformComponent>();
        for (auto e : view)
        {
            auto &interact = m_App->GetScene().registry.emplace_or_replace<UIInteractiveComponent>(e);
            interact.onClick = [](entt::entity)
            { std::cout << "Button Clicked!\n"; };
        }
    }

    void OnUpdate(float dt) override
    {
        if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_P))
        {
            std::cout << "Pause Pressed\n";
            isPaused = !isPaused;
        }

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

        if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_I))
        {
            m_App->GetSceneManager().LoadScene("scenes/placeholder2.scene");
        }

        if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_U))
        {
            m_App->GetSceneManager().UnloadScene("scenes/placeholder2.scene");
        }

        m_App->GetCameraControlSystem().Update(
            m_App->GetScene(),
            dt,
            m_App->GetKeyboard(),
            m_App->GetMouse());

        m_App->GetCameraSystem().Update(
            m_App->GetScene(),
            (float)m_App->GetWidth(),
            (float)m_App->GetHeight());

        m_App->GetUIInteractSystem().Update(
            m_App->GetScene(),
            dt,
            m_App->GetMouse());

        if (isPaused)
            return;

        m_App->GetPhysicsWorld().Update(dt);
        m_App->GetPhysicsSystem().Update(m_App->GetScene());

        m_App->GetAnimationSystem().Update(m_App->GetScene(), dt);
    }

    void OnRender() override
    {
        m_App->GetRenderSystem().Render(m_App->GetScene());

        m_App->GetUIRenderSystem().Render(
            m_App->GetScene(),
            (float)m_App->GetWidth(),
            (float)m_App->GetHeight());
    }

    void OnExit() override
    {
        m_App->GetSceneManager().UnloadScene("scenes/placeholder.scene");
    }

private:
    bool isPaused = false;

    CursorMode NextCursorMode(CursorMode mode)
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
};