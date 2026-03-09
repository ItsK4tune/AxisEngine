#include <core/logic/engine_core.h>
#include <core/logic/state_management.h>
#include <ecs/logic/entity_builder.h>
#include <ecs/manager/entity_manager.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene_manager.h>
#include <platform/logic/input_system.h>
#include <states/pause_state.h>
#include <scene/logic/scene.h>

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
        .WithUIText("PAUSED", "defaultFont", 2.0f, {1.0f, 1.0f, 1.0f})
        .Build();
}

void PauseState::OnUpdate(float dt)
{
    auto& kb = GetKeyboard();
    if (kb.IsKeyDown(Key::P))
    {
        m_Ctx.runtime->GetStateMachine().PopState();
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
