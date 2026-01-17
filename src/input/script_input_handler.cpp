#include <input/script_input_handler.h>
#include <script/scriptable.h>
#include <app/application.h> // Ensure Application is included for GetMouse, GetKeyboard
#include <functional>

void ScriptInputHandler::HandleInput(ScriptComponent& script, Scene& scene, Application* app, float dt, entt::entity entity)
{
    if (!script.instance || !script.instance->IsEnabled()) return;

    auto& mouse = app->GetMouse();
    auto& keyboard = app->GetKeyboard();

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();

    bool isHovered = false;

    if (scene.registry.all_of<UITransformComponent>(entity))
    {
        auto& transform = scene.registry.get<UITransformComponent>(entity);
        if (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
            my >= transform.position.y && my <= transform.position.y + transform.size.y)
        {
            isHovered = true;
        }
    }

    if (isHovered && !script.instance->IsHovered())
    {
        script.instance->SetHovered(true);
        script.instance->OnHoverEnter();
    }
    else if (isHovered && script.instance->IsHovered())
    {
        script.instance->OnHoverStay();
    }
    else if (!isHovered && script.instance->IsHovered())
    {
        script.instance->SetHovered(false);
        script.instance->OnHoverExit();
    }

    auto ProcessButton = [&](bool& pressedState, float& holdTimer, int buttonCode,
        std::function<void()> onClick,
        std::function<void(float)> onHold,
        std::function<void(float)> onRelease)
        {
            bool isDown = false;
            if (buttonCode == 0) isDown = mouse.IsLeftButtonPressed();
            else if (buttonCode == 1) isDown = mouse.IsRightButtonPressed();

            if (isHovered && isDown)
            {
                if (!pressedState)
                {
                    pressedState = true;
                    holdTimer = 0.0f;
                }
                else
                {
                    holdTimer += dt;
                    onHold(holdTimer);
                }
            }
            else if (pressedState && !isDown)
            {
                if (isHovered) onClick();
                onRelease(holdTimer);

                pressedState = false;
                holdTimer = 0.0f;
            }
            else if (pressedState && !isHovered && isDown)
            {
                holdTimer += dt;
                onHold(holdTimer);
            }
            else if (pressedState && !isHovered && !isDown)
            {
                onRelease(holdTimer);
                pressedState = false;
                holdTimer = 0.0f;
            }
        };

    ProcessButton(script.instance->GetLeftPressedRef(), script.instance->GetLeftHoldTimeRef(), 0,
        [&]() { script.instance->OnLeftClick(); },
        [&](float t) { script.instance->OnLeftHold(t); },
        [&](float t) { script.instance->OnLeftRelease(t); });

    ProcessButton(script.instance->GetRightPressedRef(), script.instance->GetRightHoldTimeRef(), 1,
        [&]() { script.instance->OnRightClick(); },
        [&](float t) { script.instance->OnRightHold(t); },
        [&](float t) { script.instance->OnRightRelease(t); });

    for (const auto& bind : script.instance->GetKeyBindings())
    {
        bool trigger = false;
        switch (bind.event)
        {
        case Scriptable::InputEvent::Pressed:
            trigger = keyboard.IsKeyDown(bind.key);
            break;
        case Scriptable::InputEvent::Held:
            trigger = keyboard.GetKey(bind.key);
            break;
        case Scriptable::InputEvent::Released:
            trigger = keyboard.GetKeyUp(bind.key);
            break;
        }

        if (trigger && bind.callback)
        {
            bind.callback();
        }
    }
}
