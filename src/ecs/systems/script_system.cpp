#include <ecs/system.h>
#include <script/scriptable.h>
#include <app/application.h>
#include <ecs/component.h>
#include <input/script_input_handler.h>

void ScriptableSystem::Update(Scene &scene, float dt, Application *app)
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
            script.instance->OnUpdate(dt);

            ScriptInputHandler::HandleInput(script, scene, app, dt, entity);
        }
    }
}
