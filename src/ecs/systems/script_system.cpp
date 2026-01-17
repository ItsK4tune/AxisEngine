#include <script/scriptable.h>
#include <app/application.h>
#include <ecs/component.h>
#include <input/script_input_handler.h>

void ScriptableSystem::Update(Scene &scene, float dt, float unscaledDt, Application *app)
{
    if (!m_Enabled) return;

    auto view = scene.registry.view<ScriptComponent>();
    auto& mouse = app->GetMouse();
    auto& keyboard = app->GetKeyboard();

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
            
            // If Game is Paused (dt is 0), but script can run when paused, use unscaledDt
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
