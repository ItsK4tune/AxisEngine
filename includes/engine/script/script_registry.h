#pragma once

#include <functional>
#include <memory>
#include <iostream>
#include <script/scriptable.h>
#include <string>
#include <unordered_map>
#include <utils/logger.h>

class ScriptRegistry
{
public:
    static ScriptRegistry &Instance();

    using ScriptFactory = std::function<std::unique_ptr<Scriptable>()>;

    template <typename T>
    void Register(const std::string &name)
    {
        if (m_FactoryMap.find(name) == m_FactoryMap.end())
        {
            m_FactoryMap[name] = []() -> std::unique_ptr<Scriptable>
            { return std::make_unique<T>(); };
            LOGGER_INFO("ScriptRegistry") << "Registered script: " << name;
        }
    }

    std::unique_ptr<Scriptable> Create(const std::string &name);

private:
    std::unordered_map<std::string, ScriptFactory> m_FactoryMap;
};

#define GET_MACRO(_1, _2, NAME, ...) NAME
#define REGISTER_SCRIPT(...) GET_MACRO(__VA_ARGS__, REGISTER_SCRIPT_NAMED, REGISTER_SCRIPT_DEFAULT)(__VA_ARGS__)

#define REGISTER_SCRIPT_DEFAULT(TYPE)                         \
    struct AutoRegister_##TYPE                                \
    {                                                         \
        AutoRegister_##TYPE()                                 \
        {                                                     \
            ScriptRegistry::Instance().Register<TYPE>(#TYPE); \
        }                                                     \
    };                                                        \
    static AutoRegister_##TYPE global_ver_##TYPE;

#define REGISTER_SCRIPT_NAMED(TYPE, NAME)                    \
    struct AutoRegister_##TYPE                               \
    {                                                        \
        AutoRegister_##TYPE()                                \
        {                                                    \
            ScriptRegistry::Instance().Register<TYPE>(NAME); \
        }                                                    \
    };                                                       \
    static AutoRegister_##TYPE global_ver_##TYPE;
