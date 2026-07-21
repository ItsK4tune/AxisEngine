#pragma once

#include <core/app/app_builder.h>
#include <core/app/state_machine.h>
#include <core/interface/i_application_lifecycle.h>
#include <ecs/interface/i_script_registry.h>
#include <ecs/interface/i_system_registry.h>
#include <ecs/interface/i_scriptable.h>
#include <memory>
#include <string>
#include <vector>

struct Scene;
struct AppConfig;
class StateMachine;
class RuntimeCore;
class IGraphicsContext;
class IPhysicsWorld;

class Application : public IApplicationLifecycle
{
public:
    Application();
    explicit Application(AppBuilder providers);
    virtual ~Application();

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
    virtual void RegisterUserSystems(ISystemRegistry&)
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
        GetStateMachine().PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene& GetScene();
    AppConfig GetConfig() const;
    ApplicationLifecycle GetLifecycle() const override;
    ApplicationProviderCapabilities GetProviderCapabilities() const;

private:
    static void HandleQuitSignal(int signal);
    RuntimeCore& GetRuntimeCore();
    StateMachine& GetStateMachine();
    IScriptRegistry* GetScriptRegistry();

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

template <typename T>
inline void Application::RegisterScript(const std::string& name)
{
    if (auto* reg = GetScriptRegistry())
    {
        reg->RegisterFactory(name, []() -> std::unique_ptr<IScriptable> { return std::make_unique<T>(); });
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
