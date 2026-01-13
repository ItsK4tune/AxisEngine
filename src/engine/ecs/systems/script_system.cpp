#include <engine/ecs/system.h>
#include <engine/core/scriptable.h>
#include <engine/core/application.h>

void ScriptableSystem::Update(Scene &scene, float dt, Application *app)
{
    auto view = scene.registry.view<ScriptComponent>();

    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance)
        {
            script.instance = script.InstantiateScript();
            script.instance->m_Entity = entity;
            script.instance->m_Scene = &scene;
            script.instance->m_App = app;
            script.instance->OnCreate();
        }

        if (script.instance->IsEnabled())
        {
            script.instance->OnUpdate(dt);
        }
    }
}
