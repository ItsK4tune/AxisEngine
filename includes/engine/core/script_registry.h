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

// Helper macros for optional arguments
#define GET_MACRO(_1, _2, NAME, ...) NAME
#define REGISTER_SCRIPT(...) GET_MACRO(__VA_ARGS__, REGISTER_SCRIPT_NAMED, REGISTER_SCRIPT_DEFAULT)(__VA_ARGS__)

#define REGISTER_SCRIPT_DEFAULT(TYPE) \
    struct AutoRegister_##TYPE { \
        AutoRegister_##TYPE() { \
            ScriptRegistry::Instance().Register<TYPE>(#TYPE); \
        } \
    }; \
    static AutoRegister_##TYPE global_ver_##TYPE;

#define REGISTER_SCRIPT_NAMED(TYPE, NAME) \
    struct AutoRegister_##TYPE { \
        AutoRegister_##TYPE() { \
            ScriptRegistry::Instance().Register<TYPE>(NAME); \
        } \
    }; \
    static AutoRegister_##TYPE global_ver_##TYPE;
