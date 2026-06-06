#include "test_framework.h"

#include <core/type/event_types.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/script_component.h>
#include <platform/interface/key.h>
#include <script/logic/scriptable.h>
#include <stdexcept>

namespace
{
struct ScriptState
{
    int createCount = 0;
    int updateCount = 0;
    int disableCount = 0;
    int collisionEnterCount = 0;
    int keyPressCount = 0;
    int keyReleaseCount = 0;
    float lastDt = 0.0f;
    entt::entity lastOther = entt::null;
    Key lastKey = Key::Unknown;
    bool throwOnUpdate = false;
};

class RecordingScript : public Scriptable
{
public:
    explicit RecordingScript(ScriptState& state) : m_State(state)
    {
    }

    void OnCreate() override
    {
        ++m_State.createCount;
    }

    void OnUpdate(float dt) override
    {
        if (m_State.throwOnUpdate)
            throw std::runtime_error("script update failed");
        ++m_State.updateCount;
        m_State.lastDt = dt;
    }

    void OnDisable() override
    {
        ++m_State.disableCount;
    }

    void OnCollisionEnter(entt::entity other) override
    {
        ++m_State.collisionEnterCount;
        m_State.lastOther = other;
    }

    void OnKeyPress(Key key) override
    {
        ++m_State.keyPressCount;
        m_State.lastKey = key;
    }

    void OnKeyRelease(Key key) override
    {
        ++m_State.keyReleaseCount;
        m_State.lastKey = key;
    }

private:
    ScriptState& m_State;
};

ScriptComponent& AddScript(Scene& scene, entt::entity entity, ScriptState& state)
{
    auto& script = scene.AddComponent<ScriptComponent>(entity);
    script.className = "RecordingScript";
    script.InstantiateScript = [&state]() { return std::make_unique<RecordingScript>(state); };
    script.DestroyScript = [](ScriptComponent* sc) {
        sc->instance.reset();
        sc->scriptableInstance = nullptr;
        sc->inputScriptableInstance = nullptr;
    };
    return script;
}
}  // namespace

AXIS_TEST_CASE("ScriptableSystem lazily instantiates script on first update")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState state;
    auto entity = scene.CreateEntity("Scripted");
    AddScript(scene, entity, state);

    system.Update(scene, 0.016f);

    AXIS_CHECK(state.createCount == 1);
    AXIS_CHECK(scene.GetComponent<ScriptComponent>(entity).instance != nullptr);
}

AXIS_TEST_CASE("ScriptableSystem calls OnUpdate for active enabled script")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState state;
    auto entity = scene.CreateEntity("Scripted");
    AddScript(scene, entity, state);

    system.Update(scene, 0.016f);

    AXIS_CHECK(state.updateCount == 1);
    AXIS_CHECK_NEAR(state.lastDt, 0.016f, 0.0001f);
}

AXIS_TEST_CASE("ScriptableSystem skips inactive entity script")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState state;
    auto entity = scene.CreateEntity("InactiveScripted");
    scene.GetComponent<InfoComponent>(entity).isActive = false;
    AddScript(scene, entity, state);

    system.Update(scene, 0.016f);

    AXIS_CHECK(state.createCount == 0);
    AXIS_CHECK(state.updateCount == 0);
    AXIS_CHECK(scene.GetComponent<ScriptComponent>(entity).instance == nullptr);
}

AXIS_TEST_CASE("ScriptableSystem disables throwing script")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState state;
    state.throwOnUpdate = true;
    auto entity = scene.CreateEntity("ThrowingScripted");
    AddScript(scene, entity, state);

    AXIS_EXPECT_ERROR_LOGS(1);
    system.Update(scene, 0.016f);

    auto& script = scene.GetComponent<ScriptComponent>(entity);
    AXIS_CHECK(script.instance != nullptr);
    AXIS_CHECK(!script.instance->IsEnabled());
    AXIS_CHECK(state.disableCount == 1);
}

AXIS_TEST_CASE("ScriptableSystem collision event dispatches to both scripts")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState stateA;
    ScriptState stateB;
    auto entityA = scene.CreateEntity("ScriptA");
    auto entityB = scene.CreateEntity("ScriptB");
    AddScript(scene, entityA, stateA).instance = std::make_unique<RecordingScript>(stateA);
    AddScript(scene, entityB, stateB).instance = std::make_unique<RecordingScript>(stateB);

    system.Update(scene, 0.0f);
    system.OnEntityCollision(
        EntityCollisionEvent{static_cast<uint32_t>(entityA), static_cast<uint32_t>(entityB), CollisionEventType::Enter});

    AXIS_CHECK(stateA.collisionEnterCount == 1);
    AXIS_CHECK(stateB.collisionEnterCount == 1);
    AXIS_CHECK(stateA.lastOther == entityB);
    AXIS_CHECK(stateB.lastOther == entityA);
}

AXIS_TEST_CASE("ScriptableSystem key press and release skip inactive scripts")
{
    Scene scene;
    ScriptableSystem system;
    ScriptState activeState;
    ScriptState inactiveState;
    auto active = scene.CreateEntity("ActiveScript");
    auto inactive = scene.CreateEntity("InactiveScript");
    scene.GetComponent<InfoComponent>(inactive).isActive = false;
    AddScript(scene, active, activeState).instance = std::make_unique<RecordingScript>(activeState);
    AddScript(scene, inactive, inactiveState).instance = std::make_unique<RecordingScript>(inactiveState);

    system.Update(scene, 0.0f);
    system.OnKeyPressed(KeyPressedEvent{static_cast<int>(Key::Space), 0});
    system.OnKeyReleased(KeyReleasedEvent{static_cast<int>(Key::Space), 0});

    AXIS_CHECK(activeState.keyPressCount == 1);
    AXIS_CHECK(activeState.keyReleaseCount == 1);
    AXIS_CHECK(activeState.lastKey == Key::Space);
    AXIS_CHECK(inactiveState.keyPressCount == 0);
    AXIS_CHECK(inactiveState.keyReleaseCount == 0);
}
