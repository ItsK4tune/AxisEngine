#pragma once

#include <axis_build.h>
#include <axis_commons.h>

class GameState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnFixedUpdate(float fixedDt) override;
    void OnRender() override;
    void OnExit() override;

private:
    entt::entity m_SelectedEntity = entt::null;
    float m_DayNightTimer = 0.0f;
};
