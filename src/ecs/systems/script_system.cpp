#include <ecs/systems/script_system.h>
#include <script/scriptable.h>
#include <app/application.h>
#include <input/mouse_manager.h>
#include <input/keyboard_manager.h>
#include <ecs/component.h>
#include <input/script_input_handler.h>

void ScriptableSystem::Update(Scene &scene, float dt, float unscaledDt, std::shared_ptr<Application> app)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ScriptComponent>();
    auto &mouse = app->GetMouse();
    auto &keyboard = app->GetKeyboard();

    float mx = mouse.GetLastX();
    float my = mouse.GetLastY();

    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance)
        {
            script.instance = script.InstantiateScript();
            script.instance->Init(entity, &scene, app);
            script.instance->OnCreate();
        }

        if (script.instance->IsEnabled())
        {
            float effectiveDt = dt;

            if (dt == 0.0f && script.instance->CanRunWhenPaused())
            {
                effectiveDt = unscaledDt;
            }

            if (effectiveDt > 0.0f || script.instance->CanRunWhenPaused())
            {
                script.instance->OnUpdate(effectiveDt);
                ScriptInputHandler::HandleInput(script, scene, app, effectiveDt, entity);
            }
        }
    }
}
