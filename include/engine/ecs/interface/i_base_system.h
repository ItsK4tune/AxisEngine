#pragma once

#include <string>
#include <vector>
#include <entt/entt.hpp>

class IBaseSystem {
public:
    virtual ~IBaseSystem() = default;
    
    // EngineContext removed in favor of ServiceLocator
    virtual void Initialize() {}
    virtual void Shutdown() {}
    
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual int GetPriority() const { return 0; }
    virtual std::string GetName() const = 0;

    virtual std::vector<entt::id_type> GetReadComponents() const { return {}; }
    virtual std::vector<entt::id_type> GetWriteComponents() const { return {}; }
};
