#pragma once

#include <memory>
#include <string>

class IScriptable;

class IScriptRegistry
{
public:
    virtual ~IScriptRegistry() = default;
    virtual std::unique_ptr<IScriptable> Create(const std::string& className) = 0;
};
