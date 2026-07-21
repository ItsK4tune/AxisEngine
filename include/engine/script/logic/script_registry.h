#pragma once

#include <core/logic/logger.h>
#include <ecs/interface/i_script_registry.h>
#include <script/logic/scriptable.h>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class ScriptRegistry : public IScriptRegistry
{
public:
    ScriptRegistry() = default;
    void Initialize();

    template <typename T>
    void Register(const std::string& name)
    {
        RegisterFactory(name, []() -> std::unique_ptr<IScriptable> { return std::make_unique<T>(); });
    }

    bool RegisterFactory(const std::string& name, ScriptFactory factory) override;
    std::unique_ptr<IScriptable> Create(const std::string& name) override;
    std::vector<std::string> GetRegisteredNames() const override;

private:
    std::unordered_map<std::string, ScriptFactory> m_FactoryMap;
    mutable std::mutex m_FactoryMutex;
};
