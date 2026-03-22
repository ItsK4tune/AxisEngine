#pragma once
#include <ecs/interface/i_base_system.h>

/**
 * @brief Service interface for the UI rendering system.
 */
class IUIService : virtual public IBaseSystem
{
public:
    virtual ~IUIService() = default;
};
