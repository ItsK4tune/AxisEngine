#pragma once

#include <engine/core/state.h>

class PlaceHolderState : public State
{
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnExit() override;

private:
    bool isPaused = false;
    entt::entity fpsEntity = entt::null;

    CursorMode NextCursorMode(CursorMode mode);
};