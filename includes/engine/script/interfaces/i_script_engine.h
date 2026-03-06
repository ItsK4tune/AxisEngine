#pragma once

#include <string>

class IScriptEngine
{
public:
    virtual ~IScriptEngine() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float dt) = 0;

    virtual std::string GetName() const = 0;
};
