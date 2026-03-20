#pragma once

#include <core/app/engine_accessor.h>
#include <platform/interface/key.h>
#include <platform/interface/mouse.h>
#include <entt/entt.hpp>
#include <string>
#include <ecs/unit/script_component.h>

class Scene;

class Scriptable : public EngineAccessor
{
public:
    virtual ~Scriptable() {}

    void Initialize(entt::entity entity, Scene *scene)
    {
        m_Entity = entity;
        SetActiveScene(scene);
    }

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}

    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnReset() {}

    // Event callbacks
    virtual void OnCollisionEnter(entt::entity other) {}
    virtual void OnCollisionExit(entt::entity other) {}
    virtual void OnCollisionStay(entt::entity other) {}

    virtual void OnTriggerEnter(entt::entity other) {}
    virtual void OnTriggerExit(entt::entity other) {}

    virtual void OnKeyPress(Key key) {}
    virtual void OnKeyRelease(Key key) {}
    virtual void OnMouseButtonPress(Mouse button) {}
    virtual void OnMouseButtonRelease(Mouse button) {}

    virtual void OnMouseEnter() {}
    virtual void OnMouseExit() {}
    virtual void OnMouseOver() {}
    virtual void OnMouseClicked(Mouse button) {}

    void SetEnabled(bool enabled)
    {
        if (m_Enabled == enabled)
            return;
        m_Enabled = enabled;
        if (m_Enabled)
            OnEnable();
        else
            OnDisable();
    }

    bool IsEnabled() const { return m_Enabled; }

    void SetRunWhenPaused(bool run) { m_RunWhenPaused = run; }
    bool CanRunWhenPaused() const { return m_RunWhenPaused; }

    bool CompareTag(entt::entity entity, const std::string& tag) const;
    bool CompareName(entt::entity entity, const std::string& name) const;
    std::string GetTag(entt::entity entity) const;
    std::string GetName(entt::entity entity) const;

    void SetCollisionEnabled(bool enabled);
    bool IsCollisionEnabled() const;
    void IgnoreTagCollision(const std::string& tag1, const std::string& tag2);
    void IgnoreNameCollision(const std::string& name1, const std::string& name2);

    template <typename T>
    T &GetComponent()
    {
        return GetScene().registry.get<T>(m_Entity);
    }

    template <typename T>
    bool HasComponent()
    {
        return GetScene().registry.all_of<T>(m_Entity);
    }

    template <typename T>
    T *GetScript(entt::entity targetEntity)
    {
        if (GetScene().registry.valid(targetEntity) && GetScene().registry.all_of<ScriptComponent>(targetEntity))
        {
            auto &sc = GetScene().registry.get<ScriptComponent>(targetEntity);
            if (sc.instance)
            {
                return dynamic_cast<T *>(sc.instance.get());
            }
        }
        return nullptr;
    }

protected:
    entt::entity m_Entity;
    
private:
    bool m_Enabled = true;
    bool m_RunWhenPaused = false;
};
