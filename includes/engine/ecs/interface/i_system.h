#pragma once

#include <core/unit/engine_context.h>
#include <string>
#include <vector>
#include <entt/entt.hpp>

struct Scene;

class ISystem {
public:
    virtual ~ISystem() = default;

    virtual void Initialize(EngineContext ctx) {}
    
    virtual void Update(Scene& scene, float dt) {}
    
    virtual void FixedUpdate(Scene& scene, float fixedDt) {}
    
    virtual void Render(Scene& scene) {}
    
    virtual void Shutdown() {}
    
    virtual bool IsEnabled() const = 0;
    
    virtual void SetEnabled(bool enabled) = 0;
    
    virtual int GetPriority() const { return 0; }
    
    virtual std::string GetName() const = 0;

    virtual std::vector<entt::id_type> GetReadComponents() const { return {}; }
    virtual std::vector<entt::id_type> GetWriteComponents() const { return {}; }
};