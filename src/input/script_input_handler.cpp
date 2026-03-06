#include <core/engine_context.h>
#include <window/io_handler.h>
#include <functional>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <input/script_input_handler.h>
#include <script/scriptable.h>
#include <script/input_scriptable.h>

void ScriptInputHandler::HandleInput(ScriptComponent &script, Scene &scene, EngineContext ctx, float dt, entt::entity entity)
{
    if (!script.instance || !script.instance->IsEnabled())
        return;

    auto &mouse    = ctx.io->GetMouse();
    auto &keyboard = ctx.io->GetKeyboard();

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();

    bool isHovered = false;

    if (scene.registry.all_of<UITransformComponent>(entity))
    {
        auto &transform = scene.registry.get<UITransformComponent>(entity);
        if (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
            my >= transform.position.y && my <= transform.position.y + transform.size.y)
        {
            isHovered = true;
        }
    }

    if (auto* inputScript = dynamic_cast<InputScriptable*>(script.instance.get()))
    {
        if (isHovered && !inputScript->IsHovered())
        {
            inputScript->SetHovered(true);
            inputScript->OnHoverEnter();
        }
        else if (isHovered && inputScript->IsHovered())
        {
            inputScript->OnHoverStay();
        }
        else if (!isHovered && inputScript->IsHovered())
        {
            inputScript->SetHovered(false);
            inputScript->OnHoverExit();
        }

        auto ProcessButton = [&](bool &pressedState, float &holdTimer, int mouseButton, auto onClick, auto onHold, auto onRelease)
        {
            bool isDown = (mouseButton == 0) ? mouse.IsLeftButtonPressed() : mouse.IsRightButtonPressed();

            if (!pressedState && isDown)
            {
                if (isHovered)
                {
                    pressedState = true;
                    holdTimer = 0.0f;
                }
            }
            else if (pressedState && isHovered && isDown)
            {
                holdTimer += dt;
                onHold(holdTimer);
            }
            else if (pressedState && isHovered && !isDown)
            {
                onClick();
                onRelease(holdTimer);
                pressedState = false;
                holdTimer = 0.0f;
            }
            else if (!pressedState && !isHovered && isDown)
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
                if (isHovered)
                    onClick();
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

        ProcessButton(inputScript->GetLeftPressedRef(), inputScript->GetLeftHoldTimeRef(), 0, [&]()
                      { inputScript->OnLeftClick(); }, [&](float t)
                      { inputScript->OnLeftHold(t); }, [&](float t)
                      { inputScript->OnLeftRelease(t); });

        ProcessButton(inputScript->GetRightPressedRef(), inputScript->GetRightHoldTimeRef(), 1, [&]()
                      { inputScript->OnRightClick(); }, [&](float t)
                      { inputScript->OnRightHold(t); }, [&](float t)
                      { inputScript->OnRightRelease(t); });

        for (const auto &bind : inputScript->GetKeyBindings())
        {
            bool trigger = false;
            switch (bind.event)
            {
            case InputEvent::Pressed:
                trigger = keyboard.IsKeyDown(static_cast<Input::Key>(bind.key));
                break;
            case InputEvent::Held:
                trigger = keyboard.GetKey(static_cast<Input::Key>(bind.key));
                break;
            case InputEvent::Released:
                trigger = keyboard.GetKeyUp(static_cast<Input::Key>(bind.key));
                break;
            }

            if (trigger && bind.callback)
            {
                bind.callback();
            }
        }
    }
}
