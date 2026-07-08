#pragma once

#include <core/interface/i_base_system.h>
#include <entt/entt.hpp>
#include <vector>

class IECSSystem : virtual public IBaseSystem
{
public:
    virtual ~IECSSystem() = default;

    virtual std::vector<entt::id_type> GetReadComponents() const
    {
        return {};
    }
    virtual std::vector<entt::id_type> GetWriteComponents() const
    {
        return {};
    }
};
