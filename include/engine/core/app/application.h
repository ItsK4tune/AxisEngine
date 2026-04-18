#pragma once

#include <core/logic/config_loader.h>
#include <core/logic/time_service.h>
#include <core/app/runtime_core.h>
#include <functional>
#include <platform/interface/i_window.h>
#include <script/logic/script_registry.h>
#include <memory>
#include <string>
#include <vector>

struct Scene;
class StateMachine;
class IGraphicsContext;
class IPhysicsWorld;
class Application
{
public:
    Application();
    ~Application();

    bool Initialize(const AppConfig &config);
    void Shutdown();
    void Run();
    virtual void RegisterUserScripts() {}

    template <typename T>
    void RegisterScript(const std::string &name);

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        GetRuntimeCore().PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene &GetScene();
    RuntimeCore &GetRuntimeCore();
    StateMachine &GetStateMachine();





    const AppConfig &GetConfig() const;

    ScriptRegistry* GetScriptRegistry();

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

template <typename T>
inline void Application::RegisterScript(const std::string &name)
{
    if (auto* reg = GetScriptRegistry()) {
        reg->Register<T>(name);
    }
}

class IAudioEngine;

class AppBuilder
{
public:
    static std::unique_ptr<IGraphicsContext> CreateGraphicsContext(const AppConfig &config);
    static std::unique_ptr<IAudioEngine> CreateAudioEngine(const AppConfig &config);
    static std::unique_ptr<IPhysicsWorld> CreatePhysicsWorld(const AppConfig &config);
    static std::unique_ptr<IWindow> MakeWindow();
};
