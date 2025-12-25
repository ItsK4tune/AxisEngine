#pragma once

#include <utility>
#include <game/scripts/team/team_data.h>

class TeamResourceManager
{
public:
    std::pair<bool, bool> CanConsume(const TeamStats &stats, int amount) const ;
    bool ConsumeMP(TeamStats &stats, int amount);
    bool ConsumeAP(TeamStats &stats, int amount);

    void ResetCycle(TeamStats &stats);
};