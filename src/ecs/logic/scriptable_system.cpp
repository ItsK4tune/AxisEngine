#include <platform/logic/io_handler.h>
#include <ecs/unit/core_components.h>
#include <ecs/logic/scriptable_system.h>
#include <platform/logic/input_manager.h>
#include <script/logic/scriptable.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_core.h>
#include <core/logic/service_locator.h>

void ScriptableSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ScriptComponent>();
    auto& sl = ServiceLocator::Instance();
    auto& input = sl.Require<IOHandler>();
    auto& mouse = input.GetMouse();
    auto& runtime = sl.Require<RuntimeCore>();

    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance)
        {
            script.instance = std::move(script.InstantiateScript());
            script.instance->Initialize(entity, &scene);
            script.instance->OnCreate();
        }

        if (script.instance->IsEnabled())
        {
            float effectiveDt = dt;

            if (dt == 0.0f && script.instance->CanRunWhenPaused())
            {
                effectiveDt = runtime.GetRealDeltaTime();
            }

            if (effectiveDt > 0.0f || script.instance->CanRunWhenPaused())
            {
                script.instance->OnUpdate(effectiveDt);
                ScriptInputHandler::HandleInput(script, scene, effectiveDt, entity);
            }
        }
    }
}

std::vector<entt::id_type> ScriptableSystem::GetReadComponents() const
{
    return {entt::type_id<ScriptComponent>().hash()};
}

std::vector<entt::id_type> ScriptableSystem::GetWriteComponents() const
{
    return {entt::type_id<ScriptComponent>().hash()};
}
