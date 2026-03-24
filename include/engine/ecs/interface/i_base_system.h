#pragma once

#include <string>
#include <vector>
#include <entt/entt.hpp>

class IBaseSystem {
public:
    virtual ~IBaseSystem() = default;
    

    virtual void Initialize() {}
    virtual void Shutdown() {}
    
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsSleeping() const { return false; }
    virtual void SetSleep(bool sleep) {}
    virtual int GetPriority() const { return 0; }
    virtual std::string GetName() const = 0;
};
