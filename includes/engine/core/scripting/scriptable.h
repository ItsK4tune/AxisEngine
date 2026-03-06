#pragma once

#include <core/engine_context.h>
#include <ecs/component.h>
#include <functional>
#include <scene/scene.h>
#include <core/scripting/types/script_binding.h>
#include <string>
#include <vector>

#include <core/engine_accessor.h>
class Scriptable : public EngineAccessor
{
public:
    virtual ~Scriptable() {}

    void Init(entt::entity entity, Scene *scene, EngineContext ctx)
    {
        m_Entity = entity;
        m_Ctx.scene = scene;
        m_Ctx = ctx;
    }

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}

    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnReset() {}

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
        if (GetScene().registry.all_of<ScriptComponent>(targetEntity))
        {
            auto &nsc = GetScene().registry.get<ScriptComponent>(targetEntity);
            return dynamic_cast<T *>(nsc.instance);
        }
        return nullptr;
    }

    

protected:
    entt::entity m_Entity;
    
    

private:
    bool m_Enabled = true;
    bool m_RunWhenPaused = false;

    
};
