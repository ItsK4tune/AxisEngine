#pragma once

#include <core/state.h>
#include <core/application.h>

class GameState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnExit() override;
};