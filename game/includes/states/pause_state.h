#pragma once

#include <core/logic/state_management.h>
#include <entt/entt.hpp>

class PauseState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnFixedUpdate(float fixedDt) override;
    void OnRender() override;
    void OnExit() override;

private:
    entt::entity m_PausedTextEntity = entt::null;
};
