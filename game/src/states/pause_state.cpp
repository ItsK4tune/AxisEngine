#include <states/pause_state.h>
#include <axis_component.h>
#include <axis_platform.h>
#include <axis_core.h>

void PauseState::OnEnter()
{
    EnablePhysics(false);
    EnableScript(false);
    EnableAnimation(false);
    EnableAudio(false);
    EnableParticle(false);
    EnableVideo(false);
    
    EnableRender(true);
    EnableUIRender(true);
    EnableSkybox(true);

    SetCursorMode(CursorMode::Normal);

    m_PausedTextEntity = EntityBuilder(GetScene(), GetResourceManager())
        .WithName("PauseOverlay")
        .WithUITransform({400.0f, 300.0f}, {200.0f, 50.0f})
        .WithUIText("PAUSED", "time", 2.0f, {1.0f, 1.0f, 1.0f, 1.0f})
        .Build();
}

void PauseState::OnUpdate(float dt)
{
    auto& kb = GetKeyboard();
    if (kb.IsKeyDown(Key::P))
    {
        GetRuntimeCore().GetStateMachine().PopState();
    }
}

void PauseState::OnFixedUpdate(float fixedDt) {}

void PauseState::OnRender()
{
}

void PauseState::OnExit() 
{
    if (m_PausedTextEntity != entt::null)
    {
        EntityManager::Destroy(GetScene(), m_PausedTextEntity);
        m_PausedTextEntity = entt::null;
    }
}
