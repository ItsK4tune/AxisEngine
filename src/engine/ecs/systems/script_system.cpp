#include <engine/ecs/system.h>
#include <engine/core/scriptable.h>
#include <engine/core/application.h>

void ScriptableSystem::Update(Scene &scene, float dt, Application *app)
{
    auto view = scene.registry.view<ScriptComponent>();

    for (auto entity : view)
    {
        auto &nsc = view.get<ScriptComponent>(entity);

        if (!nsc.instance)
        {
            nsc.instance = nsc.InstantiateScript();
            nsc.instance->m_Entity = entity;
            nsc.instance->m_Scene = &scene;
            nsc.instance->m_App = app;
            nsc.instance->OnCreate();
        }

        nsc.instance->OnUpdate(dt);
    }
}
