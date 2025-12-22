#pragma once

#include <map>
#include <functional>
#include <memory>
#include <game/scripts/skill_impl.h>

class SkillRegistry {
public:
    static void RegisterAll() {
        Register<TankUpSkill>("TankUp");
        Register<HeavyStrikeSkill>("HeavyStrike");
    }

    static std::shared_ptr<Skill> CreateSkill(const std::string& name) {
        if(creators.find(name) != creators.end()) {
            return creators[name]();
        }
        return nullptr;
    }

private:
    static inline std::map<std::string, std::function<std::shared_ptr<Skill>()>> creators;

    template<typename T>
    static void Register(const std::string& name) {
        creators[name] = []() { return std::make_shared<T>(); };
    }
};