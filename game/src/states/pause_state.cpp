#include <axis/axis_core.h>
#include <axis/axis_ecs.h>
#include <axis/axis_input.h>
#include <states/pause_state.h>

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

    SetCursorMode(Input::CursorMode::Normal);

    m_PausedTextEntity = EntityBuilder(GetScene(), GetResourceManager())
        .WithName("PauseOverlay")
        .WithMesh("capsuleSmoothModel", "phongLitNoShadowShader")
        .WithTransform({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f})
        .WithPhongMaterial({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 32.0f)
        .Build();
}

void PauseState::OnUpdate(float dt)
{
    auto& kb = GetKeyboard();
    if (kb.IsKeyDown(Input::Key::P))
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
