#pragma once

#include <engine/core/scene_manager.h>
#include <engine/core/resource_manager.h>
#include <engine/physic/physic_world.h>
#include <engine/core/keyboard_manager.h>
#include <engine/core/mouse_manager.h>

class Application;

class State {
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnRender() = 0;
    virtual void OnExit() = 0;

protected:
    Application* m_App = nullptr;
    void SetContext(Application* app) { m_App = app; }
    friend class StateMachine;
};