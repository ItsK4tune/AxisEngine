#pragma once

#include <string>

#include <engine/ecs/component.h>
#include <engine/core/resource_manager.h>
#include <game/scripts/unit/unit_data.h>
// #include <game/scripts/skill_registry.h>

class UnitLoader
{
public:
    // static void Load(const std::string &path, UnitStats &stats, std::vector<std::shared_ptr<Skill>> &skills, ResourceManager &resManager, MeshRendererComponent *renderer = nullptr)
    void Load(const std::string &path, UnitStats &stats, ResourceManager &resManager, MeshRendererComponent *renderer = nullptr);
};