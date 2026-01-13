#pragma once

#include <engine/core/scene.h>
#include <engine/ecs/component.h>

class Application;

class Scriptable
{
public:
    virtual ~Scriptable() {}

    void Init(entt::entity entity, Scene *scene, Application *app)
    {
        m_Entity = entity;
        m_Scene = scene;
        m_App = app;
    }

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}
    virtual void OnCollisionEnter(entt::entity other) {}
    virtual void OnCollisionStay(entt::entity other) {}
    virtual void OnCollisionExit(entt::entity other) {}

    virtual void OnTriggerEnter(entt::entity other) {}
    virtual void OnTriggerStay(entt::entity other) {}
    virtual void OnTriggerExit(entt::entity other) {}

    bool GetAction(const std::string& name);
    bool GetActionDown(const std::string& name);
    bool GetActionUp(const std::string& name);

    template <typename T>
    T &GetComponent()
    {
        return m_Scene->registry.get<T>(m_Entity);
    }

    template <typename T>
    bool HasComponent()
    {
        return m_Scene->registry.all_of<T>(m_Entity);
    }

    template <typename T>
    T *GetScript(entt::entity targetEntity)
    {
        if (m_Scene->registry.all_of<ScriptComponent>(targetEntity))
        {
            auto &nsc = m_Scene->registry.get<ScriptComponent>(targetEntity);
            return dynamic_cast<T *>(nsc.instance);
        }
        return nullptr;
    }

protected:
    entt::entity m_Entity;
    Scene *m_Scene = nullptr;
    Application *m_App = nullptr;

    friend class ScriptableSystem;
};