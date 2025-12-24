#pragma once

#include <game/scripts/level/level_model.h>

class TurnSystem
{
public:
    static void SwitchTurn(LevelModel &level, Scene* scene, bool isManualSkip);

private:
    static void AdvancePhase(LevelModel &level);
    static void ResetUnitsIfNeeded(LevelModel &level);
};