#pragma once
#include <ecs/interface/i_base_system.h>

/**
 * @brief Service interface for the skybox rendering system.
 */
class ISkyboxService : virtual public IBaseSystem
{
public:
    virtual ~ISkyboxService() = default;
};
