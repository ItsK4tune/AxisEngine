#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <core/scriptable.h>

class ScriptRegistry
{
public:
    static ScriptRegistry &Instance()
    {
        static ScriptRegistry instance;
        return instance;
    }

    using ScriptFactory = std::function<Scriptable *()>;

    template <typename T>
    void Register(const std::string &name)
    {
        if (m_FactoryMap.find(name) == m_FactoryMap.end())
        {
            m_FactoryMap[name] = []() -> Scriptable *
            { return new T(); };
            // std::cout << "[ScriptRegistry] Registered script: " << name << std::endl;
        }
    }

    Scriptable *Create(const std::string &name)
    {
        if (m_FactoryMap.find(name) != m_FactoryMap.end())
        {
            return m_FactoryMap[name]();
        }
        std::cerr << "[ScriptRegistry] Script not found: " << name << std::endl;
        return nullptr;
    }

private:
    std::unordered_map<std::string, ScriptFactory> m_FactoryMap;
};

#define REGISTER_SCRIPT(TYPE) \
    struct AutoRegister_##TYPE { \
        AutoRegister_##TYPE() { \
            ScriptRegistry::Instance().Register<TYPE>(#TYPE); \
        } \
    }; \
    static AutoRegister_##TYPE global_ver_##TYPE;
