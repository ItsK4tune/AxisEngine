#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/system_factory.h>
#include <scene/type/scene_events.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/loader_utils.h>
#include <scene/logic/component_loader.h>
#include <ecs/interface/i_script_registry.h>
#include <ecs/interface/i_scriptable.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <platform/logic/io_handler.h>
#include <core/logic/time_service.h>

REGISTER_SYSTEM(ScriptableSystem)

void ScriptableSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ScriptableSystem>(this);
    auto& ev = EventManager::Instance();

    m_SceneSubId = ev.Subscribe<SceneChangedEvent>([this](const SceneChangedEvent& e) {
        OnSceneChanged(e);
    });

    m_EventSubs.push_back(ev.Subscribe<EntityCollisionEvent>([this](const EntityCollisionEvent& e) { OnEntityCollision(e); }));
    m_EventSubs.push_back(ev.Subscribe<EntityTriggerEvent>([this](const EntityTriggerEvent& e) { OnEntityTrigger(e); }));
    m_EventSubs.push_back(ev.Subscribe<KeyPressedEvent>([this](const KeyPressedEvent& e) { OnKeyPressed(e); }));
    m_EventSubs.push_back(ev.Subscribe<KeyReleasedEvent>([this](const KeyReleasedEvent& e) { OnKeyReleased(e); }));
    m_EventSubs.push_back(ev.Subscribe<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& e) { OnMouseButtonPressed(e); }));
    m_EventSubs.push_back(ev.Subscribe<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& e) { OnMouseButtonReleased(e); }));
    m_EventSubs.push_back(ev.Subscribe<MouseMovedEvent>([this](const MouseMovedEvent& e) { OnMouseMoved(e); }));

    ComponentLoader::RegisterLoader("Script", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) {
        ScriptableSystem::LoadScript(s, e, n);
    });

    m_TimeService = &ServiceLocator::Instance().Require<TimeService>();
}

void ScriptableSystem::OnSceneChanged(const SceneChangedEvent& e)
{
    m_ActiveScene = e.scene;
    if (e.registry && m_BoundRegistries.find(e.registry) == m_BoundRegistries.end())
    {
        e.registry->on_destroy<ScriptComponent>().connect<&ScriptableSystem::OnScriptComponentDestroyed>(this);
        m_BoundRegistries.insert(e.registry);
    }
}

void ScriptableSystem::OnEntityCollision(const EntityCollisionEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;

    auto Dispatch = [&](entt::entity target, entt::entity other, CollisionEventType type) {
        if (auto sc = m_ActiveScene->registry.try_get<ScriptComponent>(target)) {
            if (sc->instance && sc->instance->IsEnabled()) {
                if (type == CollisionEventType::Enter) sc->instance->OnCollisionEnter(other);
                else if (type == CollisionEventType::Stay) sc->instance->OnCollisionStay(other);
                else if (type == CollisionEventType::Exit) sc->instance->OnCollisionExit(other);
            }
        }
    };

    Dispatch((entt::entity)e.entityA, (entt::entity)e.entityB, e.type);
    Dispatch((entt::entity)e.entityB, (entt::entity)e.entityA, e.type);
}

void ScriptableSystem::OnEntityTrigger(const EntityTriggerEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;

    auto Dispatch = [&](entt::entity target, entt::entity other, CollisionEventType type) {
        if (auto sc = m_ActiveScene->registry.try_get<ScriptComponent>(target)) {
            if (sc->instance && sc->instance->IsEnabled()) {
                if (type == CollisionEventType::Enter) sc->instance->OnTriggerEnter(other);
                else if (type == CollisionEventType::Exit) sc->instance->OnTriggerExit(other);
            }
        }
    };

    Dispatch((entt::entity)e.entityA, (entt::entity)e.entityB, e.type);
    Dispatch((entt::entity)e.entityB, (entt::entity)e.entityA, e.type);
}

void ScriptableSystem::OnKeyPressed(const KeyPressedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent>();
    for (auto entity : view) {
        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnKeyPress(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnKeyReleased(const KeyReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent>();
    for (auto entity : view) {
        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnKeyRelease(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent>();
    for (auto entity : view) {
        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnMouseButtonPress(static_cast<Mouse>(e.button));
        }
    }
}

void ScriptableSystem::OnMouseButtonReleased(const MouseButtonReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent>();
    for (auto entity : view) {
        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnMouseButtonRelease(static_cast<Mouse>(e.button));
        }
    }
}

void ScriptableSystem::OnMouseMoved(const MouseMovedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    
    auto view = m_ActiveScene->registry.view<ScriptComponent, UITransformComponent>();
    for (auto entity : view) {
        auto& sc = view.get<ScriptComponent>(entity);
        auto& ui = view.get<UITransformComponent>(entity);

        if (!sc.instance || !sc.instance->IsEnabled()) continue;

        bool isInside = (e.x >= ui.position.x && e.x <= ui.position.x + ui.size.x &&
                         e.y >= ui.position.y && e.y <= ui.position.y + ui.size.y);
        


        if (isInside) sc.instance->OnMouseOver();
    }
}

void ScriptableSystem::OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity)
{
    if (auto sc = reg.try_get<ScriptComponent>(entity))
    {
        if (sc->instance)
        {
            try {
                if (sc->DestroyScript)
                    sc->DestroyScript(sc);
                else
                    sc->instance.reset();
            } catch (...) {
                LOGGER_ERROR("ScriptableSystem") << "OnScriptComponentDestroyed: CRASH during script cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}

void ScriptableSystem::LoadScript(Scene &scene, entt::entity entity, const YAMLNode &node)
{
    LoaderUtils::ValidateKeys(node, {"Class"}, "Script");

    std::string className = node.GetChildValue("Class");
    if (className.empty())
        LOGGER_WARN("ScriptableSystem") << "Script component missing 'Class' property";

    auto& scriptComp = scene.registry.emplace<ScriptComponent>(entity);
    
    scriptComp.InstantiateScript = [className]()
    {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(className) : nullptr;
    };
    scriptComp.DestroyScript = [](ScriptComponent *nsc)
    { 
        nsc->instance.reset(); 
    };
    

}

void ScriptableSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<ScriptComponent>();
    auto& sl = ServiceLocator::Instance();

    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance)
        {
            script.instance = std::move(script.InstantiateScript());
            if (script.instance) {
                script.instance->Initialize(entity, &scene);
                script.instance->OnCreate();
            }
        }

        if (script.instance && script.instance->IsEnabled())
        {
            float effectiveDt = dt;

            if (dt == 0.0f && script.instance->CanRunWhenPaused() && m_TimeService)
            {
                effectiveDt = m_TimeService->GetRealDeltaTime();
            }

            if (effectiveDt > 0.0f || script.instance->CanRunWhenPaused())
            {
                script.instance->OnUpdate(effectiveDt);
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
