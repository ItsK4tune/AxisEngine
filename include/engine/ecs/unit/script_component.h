#pragma once

#include <ecs/interface/i_scriptable.h>
#include <entt/entt.hpp>
#include <functional>
#include <memory>

struct ScriptComponent
{
    std::string className;
    std::unique_ptr<IScriptable> instance;
    std::function<std::unique_ptr<IScriptable>()> InstantiateScript;
    std::function<void(ScriptComponent*)> DestroyScript;

    template <typename T>
    void Bind()
    {
        InstantiateScript = []() { return std::make_unique<T>(); };
        DestroyScript = [](ScriptComponent* sc) { sc->instance.reset(); };
    }
};
