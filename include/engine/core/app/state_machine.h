#pragma once

#include <core/app/engine_accessor.h>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class State : public EngineAccessor
{
public:
    virtual ~State() = default;

    virtual std::string GetName() const
    {
        return "";
    }

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt)
    {
    }
    virtual void OnRender() = 0;
    virtual void OnRenderDebug()
    {
    }
    virtual void OnExit() = 0;

    virtual void OnPause()
    {
    }
    virtual void OnResume()
    {
    }
};

enum class StateTransitionKind
{
    Push,
    Pop,
    Change,
    Custom
};

struct StateInfo
{
    std::string name;
    bool explicitlyRegistered = false;
    bool observed = false;
    bool referencedByTransition = false;
};

struct StateTransitionInfo
{
    std::string from;
    std::string to;
    std::string label;
    StateTransitionKind kind = StateTransitionKind::Custom;
    bool observed = false;
};

const char* StateTransitionKindName(StateTransitionKind kind);

class StateMachine
{
public:
    StateMachine();

    void Initialize();
    void Shutdown();

    void PushState(std::unique_ptr<State> state);
    void PopState();
    void Clear();
    void ChangeState(std::unique_ptr<State> state);

    State* GetCurrentState();
    std::vector<State*> GetStates() const;
    std::vector<StateInfo> GetRegisteredStates() const;
    std::vector<StateTransitionInfo> GetStateTransitions() const;

    void RegisterState(const std::string& name);
    void RegisterTransition(const std::string& from, const std::string& to, const std::string& label,
                            StateTransitionKind kind = StateTransitionKind::Custom);

    template <typename T>
    void RegisterState(const std::string& name = "")
    {
        RegisterStateForType(typeid(T), name.empty() ? GetStateTypeName<T>() : name, true, false, false);
    }

    template <typename From, typename To>
    void RegisterTransition(const std::string& label, StateTransitionKind kind = StateTransitionKind::Custom)
    {
        const std::string fromName = RegisterStateForType(typeid(From), GetStateTypeName<From>(), false, false, true);
        const std::string toName = RegisterStateForType(typeid(To), GetStateTypeName<To>(), false, false, true);
        RegisterTransition(fromName, toName, label, kind, false);
    }

    std::string GetStateName(const State& state) const;

    static std::string GetStateTypeName(const std::type_info& typeInfo);
    template <typename T>
    static std::string GetStateTypeName()
    {
        return GetStateTypeName(typeid(T));
    }

    void Update(float dt);
    void FixedUpdate(float fixedDt);
    void Render();

private:
    void PushStateInternal(std::unique_ptr<State> state, bool recordTransition);
    void PopStateInternal(bool recordTransition);

    std::string RegisterStateForType(const std::type_info& typeInfo, const std::string& displayName,
                                     bool explicitRegistration, bool observed, bool referencedByTransition);
    void RegisterStateName(const std::string& name, bool explicitRegistration, bool observed,
                           bool referencedByTransition);
    void RegisterTransition(const std::string& from, const std::string& to, const std::string& label,
                            StateTransitionKind kind, bool observed);
    void RenameState(const std::string& oldName, const std::string& newName);

    std::vector<std::unique_ptr<State>> m_States;
    State* m_CurrentState = nullptr;
    std::vector<StateInfo> m_StateRegistry;
    std::vector<StateTransitionInfo> m_StateTransitions;
    std::unordered_map<std::type_index, std::string> m_StateTypeNames;
};
