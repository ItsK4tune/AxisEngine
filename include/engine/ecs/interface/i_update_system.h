#pragma once
#include <ecs/interface/i_base_system.h>

struct Scene;

/**
 * @brief Interface for systems that require a logical update step.
 */
class IUpdateSystem : virtual public IBaseSystem
{
public:
    virtual ~IUpdateSystem() = default;
    virtual void Update(Scene &scene, float dt) = 0;
    virtual void FixedUpdate(Scene &scene, float fixedDt) {}
};
