#pragma once

#include <state/state.h>
#include <app/application.h>

class GameState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnFixedUpdate(float fixedDt) override;
    void OnRender() override;
    void OnExit() override;
};