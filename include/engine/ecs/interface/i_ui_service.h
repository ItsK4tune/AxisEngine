#pragma once
#include <ecs/interface/i_base_system.h>


class IUIService : virtual public IBaseSystem
{
public:
    virtual ~IUIService() = default;
};
