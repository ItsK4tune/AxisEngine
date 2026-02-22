#pragma once

#include <memory>
#include <functional>

class Scriptable;

struct ScriptComponent
{
    Scriptable *instance = nullptr;
    std::function<Scriptable *()> InstantiateScript;
    std::function<void(ScriptComponent *)> DestroyScript;

    template <typename T>
    void Bind()
    {
        InstantiateScript = []() { return static_cast<Scriptable *>(new T()); };
        DestroyScript = [](ScriptComponent *sc)
        {
            delete sc->instance;
            sc->instance = nullptr;
        };
    }
};
