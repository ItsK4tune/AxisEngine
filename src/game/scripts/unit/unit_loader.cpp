#include <game/scripts/unit/unit_loader.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <game/scripts/skill/skill.h>
#include <engine/utils/filesystem.h>
#include <game/scripts/skill/skill_registry.h>

void UnitLoader::Load(const std::string &path, UnitStats &stats, std::vector<std::shared_ptr<Skill>> &skills, ResourceManager &resManager, MeshRendererComponent *renderer)
{
    std::ifstream file(FileSystem::getPath(path).c_str());

    if (!file.is_open())
    {
        std::cerr << "[UnitLoader] Could not open unit file: " << FileSystem::getPath(path).c_str() << std::endl;
        return;
    }

    std::string line, key;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::stringstream ss(line);
        ss >> key;

        if (key == "HP")
            ss >> stats.maxHP;
        else if (key == "PHYS")
            ss >> stats.physicDmg;
        else if (key == "ELE")
            ss >> stats.elementalDmg;
        else if (key == "RANGE")
            ss >> stats.attackRange;
        else if (key == "CRIT_CHANCE")
            ss >> stats.critChance;
        else if (key == "DEF")
            ss >> stats.defense;
        else if (key == "RES")
            ss >> stats.resistance;
        else if (key == "EVAS")
            ss >> stats.evasion;
        else if (key == "GUARD")
            ss >> stats.guardBonus;
        else if (key == "SYNC")
            ss >> stats.synchronizeBonus;
        else if (key == "MOVE_RADIUS")
            ss >> stats.moveRadius;
        else if (key == "LIGHT_RADIUS")
            ss >> stats.lightRadius;
        else if (key == "MOVE_COST")
            ss >> stats.moveCost;
        else if (key == "ACTION_COST")
            ss >> stats.actionCost;

        else if (key == "MODEL")
        {
            std::string modelName;
            ss >> modelName;
            renderer->model = resManager.GetModel(modelName);
        }

        else if (key == "SHADER")
        {
            std::string shaderName;
            ss >> shaderName;
            renderer->shader = resManager.GetShader(shaderName);
        }

        else if (key == "SKILLS")
        {
            std::string skillName;
            while (ss >> skillName)
            {
                if (skillName.back() == '|')
                    skillName.pop_back();
                auto skill = SkillRegistry::CreateSkill(skillName);
                if (skill)
                    skills.push_back(skill);
            }
        }
    }
    stats.currentHP = stats.maxHP;
    std::cout << "[UnitLoader] Loaded unit from " << path << std::endl;
}