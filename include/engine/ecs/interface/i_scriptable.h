#pragma once

#include <entt/entt.hpp>
#include <string>

struct Scene;
enum class Key;
enum class Mouse;

class IScriptable
{
public:
    virtual ~IScriptable() = default;

    virtual void Initialize(entt::entity entity, Scene* scene) = 0;

    virtual void OnCreate() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) = 0;
    virtual void OnDestroy() = 0;

    virtual void OnEnable() = 0;
    virtual void OnDisable() = 0;
    virtual void OnReset() = 0;

    virtual void OnCollisionEnter(entt::entity other) = 0;
    virtual void OnCollisionExit(entt::entity other) = 0;
    virtual void OnCollisionStay(entt::entity other) = 0;

    virtual void OnTriggerEnter(entt::entity other) = 0;
    virtual void OnTriggerStay(entt::entity other) = 0;
    virtual void OnTriggerExit(entt::entity other) = 0;

    virtual void OnKeyPress(Key key) = 0;
    virtual void OnKeyRelease(Key key) = 0;
    virtual void OnMouseButtonPress(Mouse button) = 0;
    virtual void OnMouseButtonRelease(Mouse button) = 0;

    virtual void OnMouseEnter() = 0;
    virtual void OnMouseExit() = 0;
    virtual void OnMouseOver() = 0;
    virtual void OnMouseClicked(Mouse button) = 0;

    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;

    virtual void SetRunWhenPaused(bool run) = 0;
    virtual bool CanRunWhenPaused() const = 0;
};
