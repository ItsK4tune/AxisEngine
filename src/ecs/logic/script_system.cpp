#include <core/unit/engine_context.h>
#include <platform/logic/io_handler.h>
#include <ecs/unit/core_components.h>
#include <ecs/logic/script_system.h>
#include <platform/logic/input_system.h>
#include <script/logic/scriptable.h>
#include <core/logic/logger.h>
#include <core/logic/engine_core.h>
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
            script.instance->Initialize(entity, &scene, m_Ctx);
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
