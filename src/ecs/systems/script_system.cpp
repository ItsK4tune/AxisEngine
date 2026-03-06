#include <core/engine_context.h>
#include <window/io_handler.h>
#include <ecs/component.h>
#include <ecs/systems/script_system.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <input/script_input_handler.h>
#include <script/scriptable.h>
#include <utils/logger.h>
#include <core/runtime_core.h>

void ScriptableSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ScriptComponent>();
    auto &mouse = m_Ctx.io->GetMouse();
    auto &keyboard = m_Ctx.io->GetKeyboard();

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();

    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance)
        {
            script.instance = std::move(script.InstantiateScript());
            script.instance->Init(entity, &scene, m_Ctx);
            script.instance->OnCreate();
        }

        if (script.instance->IsEnabled())
        {
            float effectiveDt = dt;

            if (dt == 0.0f && script.instance->CanRunWhenPaused())
            {
                effectiveDt = m_Ctx.runtime->GetRealDeltaTime();
            }

            if (effectiveDt > 0.0f || script.instance->CanRunWhenPaused())
            {
                script.instance->OnUpdate(effectiveDt);
                ScriptInputHandler::HandleInput(script, scene, m_Ctx, effectiveDt, entity);
            }
        }
    }
}
