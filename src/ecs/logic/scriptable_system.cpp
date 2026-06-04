#include <ecs/logic/scriptable_system.h>
#include <core/logic/event_manager.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/logic/time_service.h>
#include <ecs/interface/i_script_registry.h>
#include <ecs/interface/i_scriptable.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <platform/logic/io_handler.h>
#include <scene/logic/component_loader.h>
#include <scene/type/scene_events.h>
#include <script/logic/input_scriptable.h>
#include <script/logic/scriptable.h>
#include <algorithm>

namespace
{
struct ScriptableUIRect
{
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);
};

glm::vec2 EvalUIVec(const glm::vec2& value, const glm::bvec2& isPercent, const glm::vec2& reference)
{
    return glm::vec2(isPercent.x ? (value.x * 0.01f * reference.x) : value.x,
                     isPercent.y ? (value.y * 0.01f * reference.y) : value.y);
}

ScriptableUIRect CalculateScriptableUIRect(entt::registry& registry, entt::entity entity, float screenWidth,
                                           float screenHeight)
{
    auto* transform = registry.try_get<UITransformComponent>(entity);
    if (!transform)
        return {};

    glm::vec2 parentSize(screenWidth, screenHeight);
    glm::vec2 parentPos(0.0f);

    if (auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
    {
        if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent) &&
            registry.all_of<UITransformComponent>(hierarchy->parent))
        {
            ScriptableUIRect parentRect =
                CalculateScriptableUIRect(registry, hierarchy->parent, screenWidth, screenHeight);
            parentPos = parentRect.pos;
            parentSize = parentRect.size;
        }
    }

    glm::vec2 evalAnchorMin = EvalUIVec(transform->anchorMin, transform->anchorMinIsPercent, glm::vec2(1.0f));
    glm::vec2 evalAnchorMax = EvalUIVec(transform->anchorMax, transform->anchorMaxIsPercent, glm::vec2(1.0f));

    glm::vec2 finalMin = parentPos + evalAnchorMin * parentSize +
                         EvalUIVec(transform->offsetMin, transform->offsetMinIsPercent, parentSize);
    glm::vec2 finalMax = parentPos + evalAnchorMax * parentSize +
                         EvalUIVec(transform->offsetMax, transform->offsetMaxIsPercent, parentSize);

    glm::vec2 evalPos = EvalUIVec(transform->position, transform->positionIsPercent, parentSize);
    finalMin += evalPos;
    finalMax += evalPos;

    glm::vec2 evalSize = EvalUIVec(transform->size, transform->sizeIsPercent, parentSize);
    if (evalAnchorMin.x == evalAnchorMax.x)
        finalMax.x = finalMin.x + evalSize.x;
    if (evalAnchorMin.y == evalAnchorMax.y)
        finalMax.y = finalMin.y + evalSize.y;

    return {finalMin, finalMax - finalMin};
}

template <typename ClickFn, typename HoldFn, typename ReleaseFn>
void DispatchPointerButton(bool isDown, bool canStartPress, float dt, bool& isPressed, float& holdTime,
                           ClickFn&& onClick, HoldFn&& onHold, ReleaseFn&& onRelease)
{
    if (!isPressed && isDown && canStartPress)
    {
        isPressed = true;
        holdTime = 0.0f;
        onClick();
    }

    if (!isPressed)
        return;

    if (isDown)
    {
        holdTime += dt;
        onHold(holdTime);
    }
    else
    {
        onRelease(holdTime);
        isPressed = false;
        holdTime = 0.0f;
    }
}

void DispatchUIInput(Scene& scene, float dt, const std::vector<entt::entity>& scriptEntities)
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!io)
        return;

    const float screenWidth = (std::max)(1.0f, static_cast<float>(io->GetMonitorManager().GetWidth()));
    const float screenHeight = (std::max)(1.0f, static_cast<float>(io->GetMonitorManager().GetHeight()));
    constexpr float refWidth = 1920.0f;
    constexpr float refHeight = 1080.0f;
    const float scaleFactor = (std::min)(screenWidth / refWidth, screenHeight / refHeight);
    if (scaleFactor <= 0.0f)
        return;

    const auto& mouse = io->GetMouse();
    const glm::vec2 mousePos(static_cast<float>(mouse.GetLastX()) / scaleFactor,
                             static_cast<float>(mouse.GetLastY()) / scaleFactor);
    const float safeDt = (std::max)(0.0f, dt);

    for (auto entity : scriptEntities)
    {
        if (!scene.registry.valid(entity))
            continue;

        auto* info = scene.registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* script = scene.registry.try_get<ScriptComponent>(entity);
        if (!script || !script->instance || !script->instance->IsEnabled())
            continue;

        auto* uiTransform = scene.registry.try_get<UITransformComponent>(entity);
        if (!uiTransform)
            continue;

        const ScriptableUIRect rect =
            CalculateScriptableUIRect(scene.registry, entity, screenWidth / scaleFactor, screenHeight / scaleFactor);
        const bool isInside = mousePos.x >= rect.pos.x && mousePos.x <= rect.pos.x + rect.size.x &&
                              mousePos.y >= rect.pos.y && mousePos.y <= rect.pos.y + rect.size.y;

        if (script->instance && !script->scriptableInstance)
        {
            script->scriptableInstance = dynamic_cast<Scriptable*>(script->instance.get());
            script->inputScriptableInstance = dynamic_cast<InputScriptable*>(script->instance.get());
        }

        auto* inputScript = script->inputScriptableInstance;
        if (!inputScript)
        {
            if (isInside)
                script->instance->OnMouseOver();
            continue;
        }

        if (isInside && !inputScript->IsHovered())
        {
            inputScript->SetHovered(true);
            inputScript->OnHoverEnter();
            script->instance->OnMouseEnter();
        }
        if (isInside)
        {
            inputScript->OnHoverStay();
            script->instance->OnMouseOver();
        }
        else if (inputScript->IsHovered())
        {
            inputScript->SetHovered(false);
            inputScript->OnHoverExit();
            script->instance->OnMouseExit();
        }

        DispatchPointerButton(mouse.IsLeftButtonPressed(), isInside, safeDt, inputScript->GetLeftPressedRef(),
                              inputScript->GetLeftHoldTimeRef(),
                              [&]() {
                                  inputScript->OnLeftClick();
                                  script->instance->OnMouseClicked(Mouse::Left);
                              },
                              [&](float duration) { inputScript->OnLeftHold(duration); },
                              [&](float duration) { inputScript->OnLeftRelease(duration); });

        DispatchPointerButton(mouse.IsRightButtonPressed(), isInside, safeDt, inputScript->GetRightPressedRef(),
                              inputScript->GetRightHoldTimeRef(),
                              [&]() {
                                  inputScript->OnRightClick();
                                  script->instance->OnMouseClicked(Mouse::Right);
                              },
                              [&](float duration) { inputScript->OnRightHold(duration); },
                              [&](float duration) { inputScript->OnRightRelease(duration); });

        DispatchPointerButton(mouse.IsMiddleButtonPressed(), isInside, safeDt, inputScript->GetMiddlePressedRef(),
                              inputScript->GetMiddleHoldTimeRef(),
                              [&]() {
                                  inputScript->OnMiddleClick();
                                  script->instance->OnMouseClicked(Mouse::Middle);
                              },
                              [&](float duration) { inputScript->OnMiddleHold(duration); },
                              [&](float duration) { inputScript->OnMiddleRelease(duration); });
    }
}
}  // namespace

REGISTER_SYSTEM(ScriptableSystem)

void ScriptableSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<ScriptableSystem>(this);
    auto& ev = EventManager::Instance();

    m_SceneSubId = ev.Subscribe<SceneChangedEvent>([this](const SceneChangedEvent& e) { OnSceneChanged(e); });

    m_EventSubs.push_back(
        ev.Subscribe<EntityCollisionEvent>([this](const EntityCollisionEvent& e) { OnEntityCollision(e); }));
    m_EventSubs.push_back(
        ev.Subscribe<EntityTriggerEvent>([this](const EntityTriggerEvent& e) { OnEntityTrigger(e); }));
    m_EventSubs.push_back(ev.Subscribe<KeyPressedEvent>([this](const KeyPressedEvent& e) { OnKeyPressed(e); }));
    m_EventSubs.push_back(ev.Subscribe<KeyReleasedEvent>([this](const KeyReleasedEvent& e) { OnKeyReleased(e); }));
    m_EventSubs.push_back(
        ev.Subscribe<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& e) { OnMouseButtonPressed(e); }));
    m_EventSubs.push_back(ev.Subscribe<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& e) { OnMouseButtonReleased(e); }));

    ComponentLoader::RegisterLoader("Script", [](Scene& s, entt::entity e, const YAMLNode& n, ResourceManager& r,
                                                 IPhysicsWorld* p) { ScriptableSystem::LoadScript(s, e, n); });

    m_TimeService = &ServiceLocator::Instance().Require<TimeService>();
}

void ScriptableSystem::OnSceneChanged(const SceneChangedEvent& e)
{
    m_ActiveScene = e.scene;
    if (e.registry)
    {
        if (m_BoundRegistries.find(e.registry) == m_BoundRegistries.end())
        {
            e.registry->on_construct<ScriptComponent>().connect<&ScriptableSystem::OnScriptComponentConstructed>(this);
            e.registry->on_destroy<ScriptComponent>().connect<&ScriptableSystem::OnScriptComponentDestroyed>(this);
            m_BoundRegistries.insert(e.registry);
        }

        m_ScriptEntities.clear();
        auto view = e.registry->view<ScriptComponent>();
        m_ScriptEntities.assign(view.begin(), view.end());
    }
    else
    {
        m_ScriptEntities.clear();
    }
}

void ScriptableSystem::OnScriptComponentConstructed(entt::registry& reg, entt::entity entity)
{
    if (std::find(m_ScriptEntities.begin(), m_ScriptEntities.end(), entity) == m_ScriptEntities.end())
    {
        m_ScriptEntities.push_back(entity);
    }
}

void ScriptableSystem::OnEntityCollision(const EntityCollisionEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;

    auto Dispatch = [&](entt::entity target, entt::entity other, CollisionEventType type) {
        if (auto sc = m_ActiveScene->registry.try_get<ScriptComponent>(target))
        {
            if (auto info = m_ActiveScene->registry.try_get<InfoComponent>(target))
            {
                if (!info->isActive)
                    return;
            }
            if (sc->instance && sc->instance->IsEnabled())
            {
                if (type == CollisionEventType::Enter)
                    sc->instance->OnCollisionEnter(other);
                else if (type == CollisionEventType::Stay)
                    sc->instance->OnCollisionStay(other);
                else if (type == CollisionEventType::Exit)
                    sc->instance->OnCollisionExit(other);
            }
        }
    };

    Dispatch((entt::entity)e.entityA, (entt::entity)e.entityB, e.type);
    Dispatch((entt::entity)e.entityB, (entt::entity)e.entityA, e.type);
}

void ScriptableSystem::OnEntityTrigger(const EntityTriggerEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;

    auto Dispatch = [&](entt::entity target, entt::entity other, CollisionEventType type) {
        if (auto sc = m_ActiveScene->registry.try_get<ScriptComponent>(target))
        {
            if (auto info = m_ActiveScene->registry.try_get<InfoComponent>(target))
            {
                if (!info->isActive)
                    return;
            }
            if (sc->instance && sc->instance->IsEnabled())
            {
                if (type == CollisionEventType::Enter)
                    sc->instance->OnTriggerEnter(other);
                else if (type == CollisionEventType::Stay)
                    sc->instance->OnTriggerStay(other);
                else if (type == CollisionEventType::Exit)
                    sc->instance->OnTriggerExit(other);
            }
        }
    };

    Dispatch((entt::entity)e.entityA, (entt::entity)e.entityB, e.type);
    Dispatch((entt::entity)e.entityB, (entt::entity)e.entityA, e.type);
}

void ScriptableSystem::OnKeyPressed(const KeyPressedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;
    auto entities = m_ScriptEntities;
    for (auto entity : entities)
    {
        if (!m_ActiveScene->registry.valid(entity))
            continue;
        auto* info = m_ActiveScene->registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* sc = m_ActiveScene->registry.try_get<ScriptComponent>(entity);
        if (sc && sc->instance && sc->instance->IsEnabled())
        {
            sc->instance->OnKeyPress(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnKeyReleased(const KeyReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;
    auto entities = m_ScriptEntities;
    for (auto entity : entities)
    {
        if (!m_ActiveScene->registry.valid(entity))
            continue;
        auto* info = m_ActiveScene->registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* sc = m_ActiveScene->registry.try_get<ScriptComponent>(entity);
        if (sc && sc->instance && sc->instance->IsEnabled())
        {
            sc->instance->OnKeyRelease(static_cast<Key>(e.key));
        }
    }
}

void ScriptableSystem::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;
    auto entities = m_ScriptEntities;
    for (auto entity : entities)
    {
        if (!m_ActiveScene->registry.valid(entity))
            continue;
        auto* info = m_ActiveScene->registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* sc = m_ActiveScene->registry.try_get<ScriptComponent>(entity);
        if (sc && sc->instance && sc->instance->IsEnabled())
        {
            sc->instance->OnMouseButtonPress(static_cast<Mouse>(e.button));
        }
    }
}

void ScriptableSystem::OnMouseButtonReleased(const MouseButtonReleasedEvent& e)
{
    if (!m_ActiveScene || !m_Enabled)
        return;
    auto entities = m_ScriptEntities;
    for (auto entity : entities)
    {
        if (!m_ActiveScene->registry.valid(entity))
            continue;
        auto* info = m_ActiveScene->registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* sc = m_ActiveScene->registry.try_get<ScriptComponent>(entity);
        if (sc && sc->instance && sc->instance->IsEnabled())
        {
            sc->instance->OnMouseButtonRelease(static_cast<Mouse>(e.button));
        }
    }
}

void ScriptableSystem::OnScriptComponentDestroyed(entt::registry& reg, entt::entity entity)
{
    auto it = std::find(m_ScriptEntities.begin(), m_ScriptEntities.end(), entity);
    if (it != m_ScriptEntities.end())
    {
        m_ScriptEntities.erase(it);
    }

    if (auto sc = reg.try_get<ScriptComponent>(entity))
    {
        if (sc->instance)
        {
            try
            {
                sc->instance->OnDestroy();
                if (sc->DestroyScript)
                    sc->DestroyScript(sc);
                else
                    sc->instance.reset();
            }
            catch (...)
            {
                LOGGER_ERROR("ScriptableSystem")
                    << "OnScriptComponentDestroyed: CRASH during script cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}

void ScriptableSystem::LoadScript(Scene& scene, entt::entity entity, const YAMLNode& node)
{
    LoaderUtils::ValidateKeys(node, {"Class"}, "Script");

    std::string className = node.GetChildValue("Class");
    if (className.empty() || className == "None")
        return;

    auto& scriptComp = scene.registry.emplace<ScriptComponent>(entity);
    scriptComp.className = className;

    scriptComp.InstantiateScript = [className]() {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(className) : nullptr;
    };
    scriptComp.DestroyScript = [](ScriptComponent* nsc) {
        nsc->instance.reset();
        nsc->scriptableInstance = nullptr;
        nsc->inputScriptableInstance = nullptr;
    };
}

void ScriptableSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    if (m_ActiveScene != &scene)
    {
        OnSceneChanged(SceneChangedEvent{&scene.registry, &scene});
    }

    auto entities = m_ScriptEntities;

    for (auto entity : entities)
    {
        if (!scene.registry.valid(entity))
            continue;

        auto* info = scene.registry.try_get<InfoComponent>(entity);
        if (!info || !info->isActive)
            continue;

        auto* script = scene.registry.try_get<ScriptComponent>(entity);
        if (!script)
            continue;

        if (!script->instance && script->InstantiateScript)
        {
            try
            {
                script->instance = std::move(script->InstantiateScript());
                if (script->instance)
                {
                    script->instance->Initialize(entity, &scene);
                    script->scriptableInstance = dynamic_cast<Scriptable*>(script->instance.get());
                    script->inputScriptableInstance = dynamic_cast<InputScriptable*>(script->instance.get());
                    script->instance->OnCreate();
                }
            }
            catch (const std::exception& e)
            {
                LOGGER_ERROR("ScriptableSystem")
                    << "Script Initialization CRASH on entity " << (uint32_t)entity << ": " << e.what();
                script->instance = nullptr;
            }
        }

        if (script->instance && !script->scriptableInstance)
        {
            script->scriptableInstance = dynamic_cast<Scriptable*>(script->instance.get());
            script->inputScriptableInstance = dynamic_cast<InputScriptable*>(script->instance.get());
        }

        if (script->instance && script->instance->IsEnabled())
        {
            float effectiveDt = dt;

            if (dt == 0.0f && script->instance->CanRunWhenPaused() && m_TimeService)
            {
                effectiveDt = m_TimeService->GetRealDeltaTime();
            }

            if (effectiveDt > 0.0f || script->instance->CanRunWhenPaused())
            {
                try
                {
                    if (script->scriptableInstance)
                    {
                        script->scriptableInstance->UpdateInvokes(effectiveDt);
                    }
                    script->instance->OnUpdate(effectiveDt);
                }
                catch (const std::exception& e)
                {
                    LOGGER_ERROR("ScriptableSystem")
                        << "Script Update CRASH on entity " << (uint32_t)entity << ": " << e.what();
                    script->instance->SetEnabled(false);  // Disable failing script
                }
                catch (...)
                {
                    LOGGER_ERROR("ScriptableSystem") << "Script Update UNKNOWN CRASH on entity " << (uint32_t)entity;
                    script->instance->SetEnabled(false);
                }
            }
        }
    }

    DispatchUIInput(scene, dt, m_ScriptEntities);
}

std::vector<entt::id_type> ScriptableSystem::GetReadComponents() const
{
    return {entt::type_id<ScriptComponent>().hash()};
}

std::vector<entt::id_type> ScriptableSystem::GetWriteComponents() const
{
    return {entt::type_id<ScriptComponent>().hash()};
}
