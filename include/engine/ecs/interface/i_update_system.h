#pragma once
#include <ecs/interface/i_base_system.h>

struct Scene;


class IUpdateSystem : virtual public IBaseSystem
{
public:
    virtual ~IUpdateSystem() = default;
    virtual void Update(Scene &scene, float dt) = 0;
    virtual void FixedUpdate(Scene &scene, float fixedDt) {}
};
