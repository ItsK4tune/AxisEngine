#pragma once

#include <core/engine_context.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Input { enum class CursorMode; }

#include <core/engine_accessor.h>
class State : public EngineAccessor
{
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) {}
    virtual void OnRender() = 0;
    virtual void OnExit() = 0;

protected:
};
