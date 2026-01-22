#pragma once

#ifdef ENABLE_DEBUG_SYSTEM

#include <string>

class Application;
class Scene;
class KeyboardManager;

class IDebugModule
{
public:
    virtual ~IDebugModule() = default;
    virtual void Init(Application *app) = 0;

    virtual void OnUpdate(float dt) = 0;
    virtual void Render(Scene &scene) = 0;
    virtual void ProcessInput(KeyboardManager &keyboard) = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual std::string GetModuleName() const = 0;
};

#endif
