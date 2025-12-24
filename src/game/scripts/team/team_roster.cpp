#include <game/scripts/team/team_roster.h> 

#include <game/scripts/unit/unit.h> 
#include <algorithm>
#include <engine/ecs/component.h>

void TeamRoster::Add(entt::entity unit) {
    m_Units.push_back(unit);
}

void TeamRoster::Remove(entt::entity unit) {
    m_Units.erase(std::remove(m_Units.begin(), m_Units.end(), unit), m_Units.end());
}

void TeamRoster::Clear() {
    m_Units.clear();
}

const std::vector<entt::entity>& TeamRoster::GetAll() const {
    return m_Units;
}

void TeamRoster::CleanUp(Scene* scene) {
    if (!scene) return;
    m_Units.erase(std::remove_if(m_Units.begin(), m_Units.end(),
        [scene](entt::entity e) { return !scene->registry.valid(e); }),
        m_Units.end());
}

bool TeamRoster::HasAliveUnits(Scene* scene) {
    CleanUp(scene);
    if (m_Units.empty()) return false;

    for (auto e : m_Units) {
        if (scene->registry.all_of<ScriptComponent>(e)) {
            auto& sc = scene->registry.get<ScriptComponent>(e);
            Unit* u = dynamic_cast<Unit*>(sc.instance);
            if (u && u->stats.currentHP > 0) return true;
        }
    }
    
    return false;
}