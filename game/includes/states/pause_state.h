#pragma once

#include <state/state.h>
#include <axis/axis_all.h>

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
