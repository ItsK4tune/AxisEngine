#include <states/game_state.h>
#include <states/pause_state.h>
#include <axis_component.h>
#include <axis_system.h>
#include <axis_util.h>
#include <axis_platform.h>
#include <algorithm>
#include <ecs/unit/decal_component.h>
#include <ecs/logic/decal_system.h>
#include <axis_core.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <resource/logic/resource_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/input_manager.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>

void GameState::OnEnter()
{
    this->LoadScene("scenes/main.axs");
    this->LoadScene("scenes/ui.axs");
    this->SetCursorMode(CursorMode::Normal);
    this->LoadInputBindings("resources/configs/binding.axs");

    this->EnablePhysics(true);
    this->EnableRender(true);
    this->EnableAudio(true);
    this->EnableLogic(true);

    if (auto* sysMgr = this->Resolve<SystemManager>()) {
        if (auto* rs = sysMgr->GetSystem<RenderSystem>())
            rs->SetFilterLayerMask(1);
    }
}

void GameState::OnUpdate(float dt)
{
    auto* io = this->Resolve<IOHandler>();
    if (!io) return; // headless: no input processing

    auto& input = io->GetInputManager();

    if (input.GetActionDown("LoadNextScene")) this->QueueLoadScene("scenes/game2.axs");
    if (input.GetActionDown("ReloadScene")) this->QueuePopScene();
    if (input.GetActionDown("Pause")) this->Get<RuntimeCore>().GetStateMachine().PushState(std::make_unique<PauseState>());

    if (input.GetActionDown("Select")) 
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(this->GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(this->GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(this->GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(this->GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = io->GetMouse().GetLastX();
            float mouseY = io->GetMouse().GetLastY();
            float screenW = (float)io->GetMonitorManager().GetWidth();
            float screenH = (float)io->GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / std::max(1.0f, screenW) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / std::max(1.0f, screenH);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = this->Get<IPhysicsWorld>().Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                auto* info = EntityManager::TryGetComponent<InfoComponent>(this->GetScene(), hit.entity);
                if (info && (info->tag == "ally" || info->tag == "enemy"))
                {
                    m_SelectedEntity = hit.entity;
                    LOGGER_INFO("GameState") << "Selected: " << info->name;
                }
                else
                {
                    if (m_SelectedEntity != entt::null)
                    {
                        if (auto* follower = EntityManager::TryGetComponent<PathFollowerComponent>(this->GetScene(), m_SelectedEntity))
                        {
                            follower->lockXPitch = true;
                            follower->lockZRoll = true;
                            this->GetSystem<NavigationSystem>().MoveTo(this->GetScene(), m_SelectedEntity, hit.hitPoint);
                            LOGGER_INFO("GameState") << "Move Command: target=(" << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z << ")";
                        }
                    }
                    
                    auto* decalSys = ServiceLocator::Instance().Require<SystemManager>().GetSystem<DecalSystem>();
                    if (decalSys)
                    {
                        static uint32_t crackTex = 0;
                        if (crackTex == 0) crackTex = decalSys->LoadDecalTexture("resources/textures/decal_crack.png");
                        
                        auto decalEnt = EntityManager::CreateEntity(this->GetScene(), "Decal_Crack");
                        auto& dComp = EntityManager::AddComponent<DecalComponent>(this->GetScene(), decalEnt);
                        dComp.albedoMap = crackTex;
                        dComp.lifetime = 10.0f; 
                        dComp.targetTags = {"terrain", "wall"};
                        
                        auto& dPos = EntityManager::AddComponent<PositionComponent>(this->GetScene(), decalEnt);
                        dPos.value = hit.hitPoint;
                        
                        auto& dRot = EntityManager::AddComponent<RotationComponent>(this->GetScene(), decalEnt);
                        glm::vec3 up = glm::vec3(0, 1, 0);
                        if (glm::abs(glm::dot(hit.hitNormal, up)) > 0.99f) up = glm::vec3(1, 0, 0);
                        dRot.value = glm::quatLookAt(-hit.hitNormal, up); 
                        
                        auto& dScale = EntityManager::AddComponent<ScaleComponent>(this->GetScene(), decalEnt);
                        dScale.value = glm::vec3(1.0f, 1.0f, 1.0f);
                        EntityManager::AddComponent<WorldTransformComponent>(this->GetScene(), decalEnt);
                        
                        LOGGER_INFO("GameState") << "Spawned Decal at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;

                        // Spawn particles on click too
                        // auto particleEnt = EntityManager::CreateEntity(this->GetScene(), "Impact_Particle");
                        // auto& pPos = EntityManager::AddComponent<PositionComponent>(this->GetScene(), particleEnt);
                        // pPos.value = hit.hitPoint;
                        // auto& pComp = EntityManager::AddComponent<ParticleEmitterComponent>(this->GetScene(), particleEnt);
                        
                        // pComp.isActive = true; pComp.lifetime = 0.5f;
                        // pComp.emitter.Initialize(300);
                        // pComp.emitter.Texture = this->Get<ResourceManager>().GetTextureAuto("resources/textures/particle_star.png");
                        // pComp.emitter.SpawnRate = 400.0f; pComp.emitter.LifeTime = 0.8f;   
                        // pComp.emitter.StartSize = 0.5f; pComp.emitter.EndSize = 0.0f;
                        // pComp.emitter.MinVelocity = glm::vec3(-3, 1, -3); pComp.emitter.MaxVelocity = glm::vec3(3, 6, 3);
                        // pComp.emitter.StartColor = glm::vec4(1.0f, 0.8f, 0.4f, 1.0f); 
                        // pComp.emitter.EndColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
                        // pComp.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
                        // EntityManager::AddComponent<WorldTransformComponent>(this->GetScene(), particleEnt);
                    }
                }
            }
        }
    }

    if (input.GetActionDown("ToggleBGM")) {
        auto view = this->GetScene().registry.view<InfoComponent, AudioSourceComponent>();
        for (auto e : view) {
            auto& info = view.get<InfoComponent>(e);
            auto& audio = view.get<AudioSourceComponent>(e);
            if (info.name == "BGM_Player") {
                if (audio.sound) {
                    audio.sound->Stop(); audio.sound = nullptr; audio.playOnAwake = false;
                } else audio.shouldPlay = true;
                break;
            }
        }
    }

    if (input.GetActionDown("Toggle2DSound")) {
        auto* audioSvc = this->Resolve<AudioService>();
        if (audioSvc && audioSvc->GetEngine())
            audioSvc->GetEngine()->Play2D("resources/audios/2dsound.mp3", false);
    }

    if (input.GetActionDown("VolumeUp")) {
        AppConfig cfg = this->Get<ConfigManager>().GetConfig();
        cfg.masterVolume = std::min(10.0f, cfg.masterVolume + 0.1f);
        this->Get<ConfigManager>().UpdateConfig(cfg);
        EventManager::Instance().Publish(ConfigChangedEvent{cfg});
    }
    if (input.GetActionDown("VolumeDown")) {
        AppConfig cfg = this->Get<ConfigManager>().GetConfig();
        cfg.masterVolume = std::max(0.0f, cfg.masterVolume - 0.1f);
        this->Get<ConfigManager>().UpdateConfig(cfg);
        EventManager::Instance().Publish(ConfigChangedEvent{cfg});
    }

    if (input.GetActionDown("ToggleParticle")) {
        auto view = this->GetScene().registry.view<InfoComponent, ParticleEmitterComponent>();
        for (auto e : view) {
            auto& info = view.get<InfoComponent>(e);
            if (info.name == "Particle_Demo") {
                auto& emitter = view.get<ParticleEmitterComponent>(e);
                emitter.isActive = !emitter.isActive;
                break;
            }
        }
    }

    float wheelMove = io->GetMouse().GetScrollY();
    if (std::abs(wheelMove) > 0.1f)
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(this->GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(this->GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(this->GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(this->GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = io->GetMouse().GetLastX();
            float mouseY = io->GetMouse().GetLastY();
            float screenW = (float)io->GetMonitorManager().GetWidth();
            float screenH = (float)io->GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / std::max(1.0f, screenW) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / std::max(1.0f, screenH);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = this->Get<IPhysicsWorld>().Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                if (wheelMove > 0) {
                    auto* audioSvc = this->Resolve<AudioService>();
                    if (audioSvc) audioSvc->Play3D("resources/audios/3dsound.mp3", hit.hitPoint, false);
                }
                else {
                    auto* info = EntityManager::TryGetComponent<InfoComponent>(this->GetScene(), hit.entity);
                    if (info && (info->name == "WallTest" || info->tag == "terrain"))
                    {
                        auto particleEnt = EntityManager::CreateEntity(this->GetScene(), "Impact_Particle");
                        auto& pPos = EntityManager::AddComponent<PositionComponent>(this->GetScene(), particleEnt);
                        pPos.value = hit.hitPoint;
                        auto& pComp = EntityManager::AddComponent<ParticleEmitterComponent>(this->GetScene(), particleEnt);
                        
                        pComp.isActive = true; pComp.lifetime = 0.5f;
                        pComp.emitter.Initialize(300);
                        pComp.emitter.Texture = this->Get<ResourceManager>().GetTextureAuto("resources/textures/particle_star.png");
                        pComp.emitter.SpawnRate = 400.0f; pComp.emitter.LifeTime = 0.8f;   
                        pComp.emitter.StartSize = 0.5f; pComp.emitter.EndSize = 0.0f;
                        pComp.emitter.MinVelocity = glm::vec3(-3, 1, -3); pComp.emitter.MaxVelocity = glm::vec3(3, 6, 3);
                        pComp.emitter.StartColor = glm::vec4(1.0f, 0.8f, 0.4f, 1.0f); 
                        pComp.emitter.EndColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
                        pComp.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
                        EntityManager::AddComponent<WorldTransformComponent>(this->GetScene(), particleEnt);
                    }
                }
            }
        }
    }

    auto demoView = this->GetScene().registry.view<InfoComponent, AxisMaterialComponent>();
    for (auto entity : demoView)
    {
        auto [info, mat] = demoView.get<InfoComponent, AxisMaterialComponent>(entity);
        if (info.tag == "enemy") mat.desc.ports.data[0] = 1.0f;
        else if (info.tag == "ally") mat.desc.ports.data[0] = 2.0f;
        else mat.desc.ports.data[0] = 0.0f;

        mat.desc.ports.data[1] = (entity == m_SelectedEntity) ? 1.0f : 0.0f;
    }
}

void GameState::OnFixedUpdate(float fixedDt) {}
void GameState::OnRender() {}

void GameState::OnExit()
{
    m_SelectedEntity = entt::null;
    this->Get<SceneManager>().ClearAllScenes();
}
