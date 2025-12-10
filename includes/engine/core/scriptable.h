#pragma once

#include <engine/core/scene.h>

class Application;

class Scriptable
{
public:
    virtual ~Scriptable() {}

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}

    template<typename T>
    T& GetComponent()
    {
        return m_Scene->registry.get<T>(m_Entity);
    }
    
    template<typename T>
    bool HasComponent()
    {
        return m_Scene->registry.all_of<T>(m_Entity);
    }

protected:
    entt::entity m_Entity;
    Scene* m_Scene = nullptr;
    Application* m_App = nullptr;
    
    friend class ScriptableSystem;
};