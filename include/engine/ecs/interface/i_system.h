#pragma once

#include <string>
#include <core/type/app_config.h>
#include <vector>
#include <entt/entt.hpp>

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>

struct Scene;
class IRenderStateManager;

/**
 * @brief Legacy system interface combining update and render capabilities.
 * @note Prefer inheriting from IUpdateSystem or IRenderSystem directly for better clarity.
 */
class ISystem : public IUpdateSystem, public IRenderSystem {
public:
    virtual ~ISystem() = default;
};