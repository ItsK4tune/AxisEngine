#include <core/app/state_machine.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>
#include <algorithm>

const char* StateTransitionKindName(StateTransitionKind kind)
{
    switch (kind)
    {
        case StateTransitionKind::Push:
            return "Push";
        case StateTransitionKind::Pop:
            return "Pop";
        case StateTransitionKind::Change:
            return "Change";
        case StateTransitionKind::Custom:
        default:
            return "Custom";
    }
}

StateMachine::StateMachine()
{
}

void StateMachine::Initialize()
{
}

void StateMachine::Shutdown()
{
    Clear();
}

void StateMachine::PushState(std::unique_ptr<State> state)
{
    PushStateInternal(std::move(state), true);
}

void StateMachine::PushStateInternal(std::unique_ptr<State> state, bool recordTransition)
{
    if (!m_States.empty())
    {
        m_States.back()->OnPause();
    }

    const std::string fromState = m_States.empty() ? "" : GetStateName(*m_States.back());
    const std::string toState = GetStateName(*state);
    RegisterStateName(toState, false, true, false);
    if (recordTransition && !fromState.empty())
        RegisterTransition(fromState, toState, "PushState", StateTransitionKind::Push, true);

    auto& scene = ServiceLocator::Instance().Require<Scene>();
    state->SetActiveScene(&scene);
    state->OnEnter();
    m_States.push_back(std::move(state));
    m_CurrentState = m_States.back().get();
}

void StateMachine::PopState()
{
    PopStateInternal(true);
}

void StateMachine::PopStateInternal(bool recordTransition)
{
    if (!m_States.empty())
    {
        const std::string fromState = GetStateName(*m_States.back());
        m_States.back()->OnExit();
        m_States.pop_back();
        m_CurrentState = m_States.empty() ? nullptr : m_States.back().get();
        if (!m_States.empty())
        {
            const std::string toState = GetStateName(*m_States.back());
            if (recordTransition)
                RegisterTransition(fromState, toState, "PopState", StateTransitionKind::Pop, true);
            m_States.back()->OnResume();
        }
    }
}

void StateMachine::Clear()
{
    while (!m_States.empty())
    {
        PopState();
    }
    m_CurrentState = nullptr;
}

void StateMachine::ChangeState(std::unique_ptr<State> state)
{
    const std::string fromState = m_States.empty() ? "" : GetStateName(*m_States.back());
    const std::string toState = GetStateName(*state);
    RegisterStateName(toState, false, true, false);
    if (!fromState.empty())
        RegisterTransition(fromState, toState, "ChangeState", StateTransitionKind::Change, true);

    PopStateInternal(false);
    PushStateInternal(std::move(state), false);
}

State* StateMachine::GetCurrentState()
{
    return m_CurrentState;
}

std::vector<State*> StateMachine::GetStates() const
{
    std::vector<State*> result;
    result.reserve(m_States.size());
    for (const auto& s : m_States)
    {
        result.push_back(s.get());
    }
    return result;
}

std::vector<StateInfo> StateMachine::GetRegisteredStates() const
{
    return m_StateRegistry;
}

std::vector<StateTransitionInfo> StateMachine::GetStateTransitions() const
{
    return m_StateTransitions;
}

void StateMachine::RegisterState(const std::string& name)
{
    RegisterStateName(name, true, false, false);
}

void StateMachine::RegisterTransition(const std::string& from, const std::string& to, const std::string& label,
                                      StateTransitionKind kind)
{
    RegisterTransition(from, to, label, kind, false);
}

std::string StateMachine::GetStateName(const State& state) const
{
    std::string customName = state.GetName();
    if (!customName.empty())
        return customName;

    auto it = m_StateTypeNames.find(std::type_index(typeid(state)));
    if (it != m_StateTypeNames.end())
        return it->second;
    return GetStateTypeName(typeid(state));
}

std::string StateMachine::GetStateTypeName(const std::type_info& typeInfo)
{
    std::string name = typeInfo.name();
    static const std::string classPrefix = "class ";
    static const std::string structPrefix = "struct ";
    if (name.rfind(classPrefix, 0) == 0)
        name = name.substr(classPrefix.size());
    else if (name.rfind(structPrefix, 0) == 0)
        name = name.substr(structPrefix.size());
    return name;
}

std::string StateMachine::RegisterStateForType(const std::type_info& typeInfo, const std::string& displayName,
                                               bool explicitRegistration, bool observed, bool referencedByTransition)
{
    std::type_index typeIndex(typeInfo);
    const std::string fallbackName = GetStateTypeName(typeInfo);
    const std::string resolvedName = displayName.empty() ? fallbackName : displayName;

    auto it = m_StateTypeNames.find(typeIndex);
    if (it != m_StateTypeNames.end() && it->second != resolvedName)
    {
        RenameState(it->second, resolvedName);
        it->second = resolvedName;
    }
    else if (it == m_StateTypeNames.end())
    {
        m_StateTypeNames.emplace(typeIndex, resolvedName);
        if (fallbackName != resolvedName)
            RenameState(fallbackName, resolvedName);
    }

    RegisterStateName(resolvedName, explicitRegistration, observed, referencedByTransition);
    return resolvedName;
}

void StateMachine::RegisterStateName(const std::string& name, bool explicitRegistration, bool observed,
                                     bool referencedByTransition)
{
    if (name.empty())
        return;

    auto it = std::find_if(m_StateRegistry.begin(), m_StateRegistry.end(),
                           [&](const StateInfo& info) { return info.name == name; });
    if (it == m_StateRegistry.end())
    {
        m_StateRegistry.push_back({name, explicitRegistration, observed, referencedByTransition});
        return;
    }

    it->explicitlyRegistered = it->explicitlyRegistered || explicitRegistration;
    it->observed = it->observed || observed;
    it->referencedByTransition = it->referencedByTransition || referencedByTransition;
}

void StateMachine::RegisterTransition(const std::string& from, const std::string& to, const std::string& label,
                                      StateTransitionKind kind, bool observed)
{
    if (from.empty() || to.empty())
        return;

    RegisterStateName(from, false, false, true);
    RegisterStateName(to, false, false, true);

    const std::string transitionLabel = label.empty() ? StateTransitionKindName(kind) : label;
    auto it = std::find_if(m_StateTransitions.begin(), m_StateTransitions.end(), [&](const StateTransitionInfo& t) {
        return t.from == from && t.to == to && t.label == transitionLabel && t.kind == kind;
    });
    if (it == m_StateTransitions.end())
    {
        m_StateTransitions.push_back({from, to, transitionLabel, kind, observed});
        return;
    }

    it->observed = it->observed || observed;
}

void StateMachine::RenameState(const std::string& oldName, const std::string& newName)
{
    if (oldName.empty() || newName.empty() || oldName == newName)
        return;

    auto existingNew = std::find_if(m_StateRegistry.begin(), m_StateRegistry.end(),
                                    [&](const StateInfo& info) { return info.name == newName; });
    auto existingOld = std::find_if(m_StateRegistry.begin(), m_StateRegistry.end(),
                                    [&](const StateInfo& info) { return info.name == oldName; });

    if (existingOld != m_StateRegistry.end())
    {
        if (existingNew != m_StateRegistry.end())
        {
            existingNew->explicitlyRegistered = existingNew->explicitlyRegistered || existingOld->explicitlyRegistered;
            existingNew->observed = existingNew->observed || existingOld->observed;
            existingNew->referencedByTransition =
                existingNew->referencedByTransition || existingOld->referencedByTransition;
            m_StateRegistry.erase(existingOld);
        }
        else
        {
            existingOld->name = newName;
        }
    }

    for (auto& transition : m_StateTransitions)
    {
        if (transition.from == oldName)
            transition.from = newName;
        if (transition.to == oldName)
            transition.to = newName;
    }
}

void StateMachine::Update(float dt)
{
    if (State* s = GetCurrentState())
        s->OnUpdate(dt);
}

void StateMachine::FixedUpdate(float fixedDt)
{
    if (State* s = GetCurrentState())
        s->OnFixedUpdate(fixedDt);
}

void StateMachine::Render()
{
    if (State* s = GetCurrentState())
        s->OnRender();
}
