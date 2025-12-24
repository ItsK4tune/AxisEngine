#pragma once

#include <engine/core/scene.h>
#include <game/scripts/level/level_model.h>

class VisionSystem {
public:
    static void UpdateFogOfWar(LevelModel& level, Scene* scene);
};