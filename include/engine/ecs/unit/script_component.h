#pragma once

#include <memory>
#include <functional>
#include <entt/entt.hpp>

// Forward declarations
class IScriptable;

// --- Script ---

struct ScriptComponent
{
    std::unique_ptr<IScriptable> instance;
    std::function<std::unique_ptr<IScriptable>()> InstantiateScript;
    std::function<void(ScriptComponent *)> DestroyScript;

    template <typename T>
    void Bind()
    {
        InstantiateScript = []() { return std::make_unique<T>(); };
        DestroyScript = [](ScriptComponent *sc)
        {
            sc->instance.reset();
        };
    }
};
