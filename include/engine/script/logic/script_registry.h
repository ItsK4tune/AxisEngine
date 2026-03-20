#pragma once

#include <core/logic/logger.h>
#include <functional>
#include <iostream>
#include <memory>
#include <script/logic/scriptable.h>
#include <string>
#include <unordered_map>

class ScriptRegistry
{
public:
    ScriptRegistry() = default;
    void Initialize();

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

public:
    static std::unordered_map<std::string, ScriptFactory>& GetStaticFactoryMap() {
        static std::unordered_map<std::string, ScriptFactory> staticMap;
        return staticMap;
    }
};

#define GET_MACRO(_1, _2, NAME, ...) NAME
#define REGISTER_SCRIPT(...) GET_MACRO(__VA_ARGS__, REGISTER_SCRIPT_NAMED, REGISTER_SCRIPT_DEFAULT)(__VA_ARGS__)

#define REGISTER_SCRIPT_DEFAULT(TYPE)                         \
    struct AutoRegister_##TYPE                                \
    {                                                         \
        AutoRegister_##TYPE()                                 \
        {                                                     \
            ScriptRegistry::GetStaticFactoryMap()[#TYPE] = []() -> std::unique_ptr<Scriptable> \
            { return std::make_unique<TYPE>(); };             \
        }                                                     \
    };                                                        \
    static AutoRegister_##TYPE global_ver_##TYPE;

#define REGISTER_SCRIPT_NAMED(TYPE, NAME)                    \
    struct AutoRegister_##TYPE                               \
    {                                                        \
        AutoRegister_##TYPE()                                \
        {                                                    \
            ScriptRegistry::GetStaticFactoryMap()[NAME] = []() -> std::unique_ptr<Scriptable> \
            { return std::make_unique<TYPE>(); };            \
        }                                                    \
    };                                                       \
    static AutoRegister_##TYPE global_ver_##TYPE;