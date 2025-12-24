#pragma once

#include <vector>
#include <entt/entt.hpp>

#include <engine/core/scene.h>

class TeamRoster {
public:
    void Add(entt::entity unit);
    void Remove(entt::entity unit);
    void Clear();
    
    void CleanUp(Scene* scene);
    bool HasAliveUnits(Scene* scene);

    const std::vector<entt::entity>& GetAll() const;

private:
    std::vector<entt::entity> m_Units;
};