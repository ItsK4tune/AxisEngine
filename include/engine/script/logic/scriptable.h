#pragma once

#include <core/app/engine_accessor.h>
#include <core/logic/data_node.h>
#include <ecs/interface/i_scriptable.h>
#include <ecs/unit/script_component.h>
#include <platform/interface/key.h>
#include <platform/interface/mouse.h>
#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <vector>

class Scriptable : public EngineAccessor, public IScriptable
{
public:
    virtual ~Scriptable()
    {
    }

    void Initialize(entt::entity entity, Scene* scene)
    {
        m_Entity = entity;
        SetActiveScene(scene);
    }

    virtual void OnCreate()
    {
    }
    virtual void OnUpdate(float dt)
    {
    }
    virtual void OnFixedUpdate(float fixedDt) override
    {
    }
    virtual void OnDestroy()
    {
    }

    virtual void OnEnable()
    {
    }
    virtual void OnDisable()
    {
    }
    virtual void OnReset()
    {
    }

    virtual void OnCollisionEnter(entt::entity other)
    {
    }
    virtual void OnCollisionExit(entt::entity other)
    {
    }
    virtual void OnCollisionStay(entt::entity other)
    {
    }

    virtual void OnTriggerEnter(entt::entity other) override
    {
    }
    virtual void OnTriggerStay(entt::entity other) override
    {
    }
    virtual void OnTriggerExit(entt::entity other) override
    {
    }

    virtual void OnKeyPress(Key key)
    {
    }
    virtual void OnKeyRelease(Key key)
    {
    }
    virtual void OnMouseButtonPress(Mouse button)
    {
    }
    virtual void OnMouseButtonRelease(Mouse button)
    {
    }

    virtual void OnMouseEnter()
    {
    }
    virtual void OnMouseExit()
    {
    }
    virtual void OnMouseOver()
    {
    }
    virtual void OnMouseClicked(Mouse button)
    {
    }

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

    bool IsEnabled() const
    {
        return m_Enabled;
    }

    void SetRunWhenPaused(bool run)
    {
        m_RunWhenPaused = run;
    }
    bool CanRunWhenPaused() const
    {
        return m_RunWhenPaused;
    }

    bool CompareTag(entt::entity entity, const std::string& tag) const;
    bool CompareName(entt::entity entity, const std::string& name) const;
    std::string GetTag(entt::entity entity) const;
    std::string GetName(entt::entity entity) const;

    void SetCollisionEnabled(bool enabled);
    bool IsCollisionEnabled() const;
    void IgnoreTagCollision(const std::string& tag1, const std::string& tag2);
    void IgnoreNameCollision(const std::string& name1, const std::string& name2);

    void SetDataNode(const std::string& key, const DataNode& data);
    DataNode GetDataNode(const std::string& key) const;
    bool HasDataNode(const std::string& key) const;
    void SetDataNodeValue(const std::string& key, const std::string& value);
    std::string GetDataNodeValue(const std::string& key, const std::string& defaultVal = "") const;

    template <typename T>
    T& GetComponent()
    {
        return GetScene().GetComponent<T>(m_Entity);
    }

    template <typename T>
    bool HasComponent()
    {
        return GetScene().HasAllComponents<T>(m_Entity);
    }

    template <typename T>
    T* GetScript(entt::entity targetEntity)
    {
        if (GetScene().IsValid(targetEntity) && GetScene().HasAllComponents<ScriptComponent>(targetEntity))
        {
            auto& sc = GetScene().GetComponent<ScriptComponent>(targetEntity);
            if (sc.instance)
            {
                return dynamic_cast<T*>(sc.instance.get());
            }
        }
        return nullptr;
    }

    entt::entity Spawn(const std::string& name = "unnamed", const std::string& tag = "default");
    entt::entity Spawn(const std::string& name, const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f),
                       const glm::vec3& scale = glm::vec3(1.0f));
    void Destroy(entt::entity entity);

    void Invoke(std::function<void()> callback, float delay);
    void UpdateInvokes(float dt);

protected:
    entt::entity m_Entity;

private:
    struct PendingInvoke
    {
        std::function<void()> callback;
        float delay;
    };
    std::vector<PendingInvoke> m_PendingInvokes;

    bool m_Enabled = true;
    bool m_RunWhenPaused = false;
};
