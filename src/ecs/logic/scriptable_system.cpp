#include <ecs/logic/scriptable_system.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/logic/logger.h>
#include <core/logic/loader_utils.h>
#include <scene/logic/component_loader.h>
#include <script/logic/script_registry.h>
#include <ecs/unit/script_component.h>
#include <script/logic/scriptable.h>
#include <ecs/unit/ui_components.h>
#include <platform/logic/io_handler.h>
#include <script/logic/input_scriptable.h>
#include <core/app/runtime_core.h>

void ScriptableSystem::Initialize()
{
    m_SceneSubId = EventSystem::Instance().Subscribe<SceneChangedEvent>([this](const SceneChangedEvent& e) {
        OnSceneChanged(e);
    });

    ComponentLoader::RegisterLoader("Script", [](Scene &s, entt::entity e, const YAMLNode &n, ResourceManager &r, IPhysicsWorld *p) {
        ScriptableSystem::LoadScript(s, e, n);
    });
}

void ScriptableSystem::OnSceneChanged(const SceneChangedEvent& e)
{
    if (e.registry && m_BoundRegistries.find(e.registry) == m_BoundRegistries.end())
    {
        e.registry->on_destroy<ScriptComponent>().connect<&ScriptableSystem::OnScriptComponentDestroyed>(this);
        m_BoundRegistries.insert(e.registry);
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
        auto registry = ServiceLocator::Instance().Resolve<ScriptRegistry>();
        return registry ? registry->Create(className) : nullptr;
    };
    scriptComp.DestroyScript = [](ScriptComponent *nsc)
    { 
        nsc->instance.reset(); 
    };
    
    // Note: instance will be created in the first Update()
}

namespace {
    void HandleScriptInput(ScriptComponent &script, Scene &scene, float dt, entt::entity entity)
    {
        if (!script.instance || !script.instance->IsEnabled())
            return;

        auto &sl       = ServiceLocator::Instance();
        auto &io       = sl.Require<IOHandler>();
        auto &mouse    = io.GetMouse();
        auto &keyboard = io.GetKeyboard();

        float mx = mouse.GetLastX();
        float my = mouse.GetLastY();

        bool isHovered = false;

        if (scene.registry.all_of<UITransformComponent>(entity))
        {
            auto &transform = scene.registry.get<UITransformComponent>(entity);
            if (mx >= transform.position.x && mx <= transform.position.x + transform.size.x &&
                my >= transform.position.y && my <= transform.position.y + transform.size.y)
            {
                isHovered = true;
            }
        }

        if (auto* inputScript = dynamic_cast<InputScriptable*>(script.instance.get()))
        {
            if (isHovered && !inputScript->IsHovered())
            {
                inputScript->SetHovered(true);
                inputScript->OnHoverEnter();
            }
            else if (isHovered && inputScript->IsHovered())
            {
                inputScript->OnHoverStay();
            }
            else if (!isHovered && inputScript->IsHovered())
            {
                inputScript->SetHovered(false);
                inputScript->OnHoverExit();
            }

            auto ProcessButton = [&](bool &pressedState, float &holdTimer, int mouseButton, auto onClick, auto onHold, auto onRelease)
            {
                bool isDown = (mouseButton == 0) ? mouse.IsLeftButtonPressed() : mouse.IsRightButtonPressed();

                if (!pressedState && isDown)
                {
                    if (isHovered)
                    {
                        pressedState = true;
                        holdTimer = 0.0f;
                    }
                }
                else if (pressedState && isHovered && isDown)
                {
                    holdTimer += dt;
                    onHold(holdTimer);
                }
                else if (pressedState && isHovered && !isDown)
                {
                    onClick();
                    onRelease(holdTimer);
                    pressedState = false;
                    holdTimer = 0.0f;
                }
                else if (!pressedState && !isHovered && isDown)
                {
                    if (!pressedState)
                    {
                        pressedState = true;
                        holdTimer = 0.0f;
                    }
                    else
                    {
                        holdTimer += dt;
                        onHold(holdTimer);
                    }
                }
                else if (pressedState && !isDown)
                {
                    if (isHovered)
                        onClick();
                    onRelease(holdTimer);

                    pressedState = false;
                    holdTimer = 0.0f;
                }
                else if (pressedState && !isHovered && isDown)
                {
                    holdTimer += dt;
                    onHold(holdTimer);
                }
                else if (pressedState && !isHovered && !isDown)
                {
                    onRelease(holdTimer);
                    pressedState = false;
                    holdTimer = 0.0f;
                }
            };

            ProcessButton(inputScript->GetLeftPressedRef(), inputScript->GetLeftHoldTimeRef(), 0, [&]()
                          { inputScript->OnLeftClick(); }, [&](float t)
                          { inputScript->OnLeftHold(t); }, [&](float t)
                          { inputScript->OnLeftRelease(t); });

            ProcessButton(inputScript->GetRightPressedRef(), inputScript->GetRightHoldTimeRef(), 1, [&]()
                          { inputScript->OnRightClick(); }, [&](float t)
                          { inputScript->OnRightHold(t); }, [&](float t)
                          { inputScript->OnRightRelease(t); });

            for (const auto &bind : inputScript->GetKeyBindings())
            {
                bool trigger = false;
                switch (bind.event)
                {
                case InputEvent::Pressed:
                    trigger = keyboard.IsKeyDown(static_cast<Key>(bind.key));
                    break;
                case InputEvent::Held:
                    trigger = keyboard.GetKey(static_cast<Key>(bind.key));
                    break;
                case InputEvent::Released:
                    trigger = keyboard.GetKeyUp(static_cast<Key>(bind.key));
                    break;
                }

                if (trigger && bind.callback)
                {
                    bind.callback();
                }
            }
        }
    }
}

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
                HandleScriptInput(script, scene, effectiveDt, entity);
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
