#include <game/scripts/team/team_resource_manager.h>
#include <iostream>

std::pair<bool, bool> TeamResourceManager::CheckCanConsume(const TeamStats &stats, int amount) const
{
    return std::make_pair(
        stats.currentMP >= amount,
        stats.currentAP >= amount);
}

bool TeamResourceManager::ConsumeMP(TeamStats &stats, int amount)
{
    if (stats.currentMP >= amount)
    {
        stats.currentMP -= amount;
        return true;
    }

    return false;
}

bool TeamResourceManager::ConsumeAP(TeamStats &stats, int amount)
{
    if (stats.currentAP >= amount)
    {
        stats.currentAP -= amount;
        return true;
    }

    return false;
}

void TeamResourceManager::ResetCycle(TeamStats &stats)
{
    stats.currentMP = stats.maxMP;
    stats.currentAP = stats.maxAP;
    std::cout << "[TeamResourceManager] Resources Refreshed.\n";
}