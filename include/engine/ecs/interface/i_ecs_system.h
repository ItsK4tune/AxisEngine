#pragma once

#include <ecs/interface/i_base_system.h>
#include <vector>
#include <entt/entt.hpp>

/**
 * @brief Interface for systems that interact with ECS components.
 * Provides metadata for dependency sorting and parallel execution.
 */
class IECSSystem : virtual public IBaseSystem {
public:
    virtual ~IECSSystem() = default;

    virtual std::vector<entt::id_type> GetReadComponents() const { return {}; }
    virtual std::vector<entt::id_type> GetWriteComponents() const { return {}; }
};
