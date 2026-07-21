#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class IScriptable;

class IScriptRegistry
{
public:
    using ScriptFactory = std::function<std::unique_ptr<IScriptable>()>;

    virtual ~IScriptRegistry() = default;
    virtual bool RegisterFactory(const std::string& className, ScriptFactory factory) = 0;
    virtual std::unique_ptr<IScriptable> Create(const std::string& className) = 0;
    virtual std::vector<std::string> GetRegisteredNames() const = 0;
};
