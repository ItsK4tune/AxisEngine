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
#include <ecs/unit/core_components.h>
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
            if (auto info = m_ActiveScene->registry.try_get<InfoComponent>(target)) {
                if (!info->isActive) return;
            }
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
            if (auto info = m_ActiveScene->registry.try_get<InfoComponent>(target)) {
                if (!info->isActive) return;
            }
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
    auto view = m_ActiveScene->registry.view<ScriptComponent, InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive) continue;

        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnKeyPress(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnKeyReleased(const KeyReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent, InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive) continue;

        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnKeyRelease(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent, InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive) continue;

        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnMouseButtonPress(static_cast<Mouse>(e.button));
        }
    }
}

void ScriptableSystem::OnMouseButtonReleased(const MouseButtonReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled) return;
    auto view = m_ActiveScene->registry.view<ScriptComponent, InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive) continue;

        auto& sc = view.get<ScriptComponent>(entity);
        if (sc.instance && sc.instance->IsEnabled()) {
            sc.instance->OnMouseButtonRelease(static_cast<Mouse>(e.button));
        }
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
    scriptComp.className = className;
    
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

    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        const auto& mouse = io->GetMouse();
        if (mouse.GetXOffset() != 0.0f || mouse.GetYOffset() != 0.0f)
        {
            double mouseX = mouse.GetLastX();
            double mouseY = mouse.GetLastY();

            auto uiView = scene.registry.view<ScriptComponent, UITransformComponent, InfoComponent>();
            for (auto entity : uiView)
            {
                auto& info = uiView.get<InfoComponent>(entity);
                if (!info.isActive) continue;

                auto& sc = uiView.get<ScriptComponent>(entity);
                auto& ui = uiView.get<UITransformComponent>(entity);

                if (!sc.instance || !sc.instance->IsEnabled()) continue;

                bool isInside = (mouseX >= ui.position.x && mouseX <= ui.position.x + ui.size.x &&
                                 mouseY >= ui.position.y && mouseY <= ui.position.y + ui.size.y);
                
                if (isInside)
                {
                    sc.instance->OnMouseOver();
                }
            }
        }
    }

    auto view = scene.registry.view<ScriptComponent, InfoComponent>();
    auto& sl = ServiceLocator::Instance();

    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive) continue;

        auto &script = view.get<ScriptComponent>(entity);

        if (!script.instance && script.InstantiateScript)
        {
            try {
                script.instance = std::move(script.InstantiateScript());
                if (script.instance) {
                    script.instance->Initialize(entity, &scene);
                    script.instance->OnCreate();
                }
            } catch (const std::exception& e) {
                LOGGER_ERROR("ScriptableSystem") << "Script Initialization CRASH on entity " << (uint32_t)entity << ": " << e.what();
                script.instance = nullptr; 
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
                try {
                    script.instance->OnUpdate(effectiveDt);
                } catch (const std::exception& e) {
                    LOGGER_ERROR("ScriptableSystem") << "Script Update CRASH on entity " << (uint32_t)entity << ": " << e.what();
                    script.instance->SetEnabled(false); // Disable failing script
                } catch (...) {
                    LOGGER_ERROR("ScriptableSystem") << "Script Update UNKNOWN CRASH on entity " << (uint32_t)entity;
                    script.instance->SetEnabled(false);
                }
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
