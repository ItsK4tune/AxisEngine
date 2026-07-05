#pragma once

#include <core/app/runtime_core.h>
#include <core/logic/config_loader.h>
#include <core/logic/time_service.h>
#include <platform/interface/i_window.h>
#include <script/logic/script_registry.h>
#include <functional>
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

    bool Initialize();
    bool Initialize(const AppConfig& config);
    void Shutdown();
    void Run();
    virtual void RegisterUserStates()
    {
    }
    virtual void RegisterUserScripts()
    {
    }

    template <typename T>
    void RegisterScript(const std::string& name);
    template <typename T>
    void RegisterState(const std::string& name = "");
    template <typename From, typename To>
    void RegisterStateTransition(const std::string& label, StateTransitionKind kind = StateTransitionKind::Custom);

    template <typename T, typename... Args>
    void PushState(Args&&... args)
    {
        GetRuntimeCore().PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene& GetScene();
    RuntimeCore& GetRuntimeCore();
    StateMachine& GetStateMachine();

    AppConfig GetConfig() const;

    ScriptRegistry* GetScriptRegistry();

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

template <typename T>
inline void Application::RegisterScript(const std::string& name)
{
    if (auto* reg = GetScriptRegistry())
    {
        reg->Register<T>(name);
    }
}

template <typename T>
inline void Application::RegisterState(const std::string& name)
{
    GetStateMachine().RegisterState<T>(name);
}

template <typename From, typename To>
inline void Application::RegisterStateTransition(const std::string& label, StateTransitionKind kind)
{
    GetStateMachine().RegisterTransition<From, To>(label, kind);
}

class IAudioEngine;

class AppBuilder
{
public:
    static std::unique_ptr<IGraphicsContext> CreateGraphicsContext(const AppConfig& config);
    static std::unique_ptr<IAudioEngine> CreateAudioEngine(const AppConfig& config);
    static std::unique_ptr<IPhysicsWorld> CreatePhysicsWorld(const AppConfig& config);
    static std::unique_ptr<IWindow> MakeWindow();
};
