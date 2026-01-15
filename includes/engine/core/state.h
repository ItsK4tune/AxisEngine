#pragma once

#include <core/scene_manager.h>
#include <core/resource_manager.h>
#include <core/keyboard_manager.h>
#include <core/mouse_manager.h>
#include <physic/physic_world.h>

class Application;

class State {
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) {}
    virtual void OnRender() = 0;
    virtual void OnExit() = 0;

protected:
    Application* m_App = nullptr;
    void SetContext(Application* app) { m_App = app; }
    friend class StateMachine;
};