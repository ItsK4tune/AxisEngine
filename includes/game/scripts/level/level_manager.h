#pragma once
#include <engine/core/scriptable.h>
#include <game/scripts/level/level_model.h>
#include <game/scripts/level/level_generator.h>

class LevelManager : public Scriptable {
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;

private:
    LevelModel m_Level;
};