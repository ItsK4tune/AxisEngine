#include <states/pause_state.h>
#include <axis_component.h>
#include <axis_platform.h>
#include <axis_core.h>

#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/entity_builder.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>
#include <platform/logic/io_handler.h>
#include <scene/logic/scene_manager.h>

void PauseState::OnEnter()
{
    this->EnablePhysics(false);
    this->EnableScript(false);
    this->EnableAnimation(false);
    this->EnableAudio(false);
    this->EnableParticle(false);
    this->EnableVideo(false);
    
    this->EnableRender(true);
    this->EnableUIRender(true);
    this->EnableSkybox(true);

    this->SetCursorMode(CursorMode::Normal);

    m_PausedTextEntity = EntityBuilder(this->GetScene(), this->Get<ResourceManager>())
        .WithName("PauseOverlay")
        .WithUITransform({400.0f, 300.0f}, {200.0f, 50.0f})
        .WithUIText("PAUSED", "time", 2.0f, {1.0f, 1.0f, 1.0f, 1.0f})
        .Build();
}

void PauseState::OnUpdate(float dt)
{
    auto& kb = this->Get<IOHandler>().GetKeyboard();
    if (kb.IsKeyDown(Key::P))
    {
        this->Get<RuntimeCore>().GetStateMachine().PopState();
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
        EntityManager::Destroy(this->GetScene(), m_PausedTextEntity);
        m_PausedTextEntity = entt::null;
    }
}
