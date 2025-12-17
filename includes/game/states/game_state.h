#pragma once

#include <engine/core/state.h>
#include <engine/core/application.h>

class TacticsState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnExit() override;
};