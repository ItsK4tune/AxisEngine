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

void GameState::OnEnter()
{
    this->LoadScene("scenes/game3.axs");
    this->LoadScene("scenes/ui.axs");
    this->SetCursorMode(CursorMode::Normal);
    this->LoadInputBindings("resources/configs/binding.axs");

    this->EnablePhysics(true);
    this->EnableRender(true);
    this->EnableAudio(true);
    this->EnableLogic(true);

    this->GetRenderSystem().SetFilterLayerMask(1);
}

void GameState::OnUpdate(float dt)
{
    auto& input = this->GetInputManager();

    if (input.GetActionDown("LoadNextScene")) this->QueueLoadScene("scenes/game2.axs");
    if (input.GetActionDown("ReloadScene")) this->QueuePopScene();
    if (input.GetActionDown("Pause")) this->GetRuntimeCore().GetStateMachine().PushState(std::make_unique<PauseState>());

    // Selection & Movement Logic (Combined into Mouse Left Click)
    if (input.GetActionDown("Select")) 
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(this->GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(this->GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(this->GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(this->GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = this->GetMouse().GetLastX();
            float mouseY = this->GetMouse().GetLastY();
            float screenW = (float)this->GetIOHandler().GetMonitorManager().GetWidth();
            float screenH = (float)this->GetIOHandler().GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / (screenW > 0 ? screenW : 1.0f) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / (screenH > 0 ? screenH : 1.0f);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = this->GetPhysicsSystem().GetPhysicsWorld().Raycast(camPos, rayDir, 1000.0f);
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
                    // Move command IF an entity is selected
                    if (m_SelectedEntity != entt::null)
                    {
                        if (auto* follower = EntityManager::TryGetComponent<PathFollowerComponent>(this->GetScene(), m_SelectedEntity))
                        {
                            follower->lockXPitch = true;
                            follower->lockZRoll = true;
                            this->GetNavigationSystem().MoveTo(this->GetScene(), m_SelectedEntity, hit.hitPoint);
                            LOGGER_INFO("GameState") << "Move Command: target=(" << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z << ") entity=" << (uint32_t)m_SelectedEntity;
                        }
                    }
                    
                    // ALWAYS spawn decal on click if we hit something (terrain, etc)
                    auto* decalSys = ServiceLocator::Instance().Require<SystemManager>().GetSystem<DecalSystem>();
                    if (decalSys)
                    {
                        static uint32_t crackTex = 0;
                        if (crackTex == 0) crackTex = decalSys->LoadDecalTexture("resources/textures/decal_crack.png");
                        
                        auto decalEnt = EntityManager::CreateEntity(this->GetScene(), "Decal_Crack");
                        auto& dComp = EntityManager::AddComponent<DecalComponent>(this->GetScene(), decalEnt);
                        dComp.albedoMap = crackTex;
                        dComp.lifetime = 10.0f; // Fade out after 10s
                        dComp.targetTags = {"terrain", "wall"}; // API demo: stick to terrain and wall
                        
                        auto& dPos = EntityManager::AddComponent<PositionComponent>(this->GetScene(), decalEnt);
                        dPos.value = hit.hitPoint;
                        
                        auto& dRot = EntityManager::AddComponent<RotationComponent>(this->GetScene(), decalEnt);
                        // Align decal box to surface normal
                        glm::vec3 up = glm::vec3(0, 1, 0);
                        if (glm::abs(glm::dot(hit.hitNormal, up)) > 0.99f) up = glm::vec3(1, 0, 0);
                        dRot.value = glm::quatLookAt(-hit.hitNormal, up); // Decal projects along -Z
                        
                        auto& dScale = EntityManager::AddComponent<ScaleComponent>(this->GetScene(), decalEnt);
                        dScale.value = glm::vec3(1.0f, 1.0f, 1.0f);
                        
                        EntityManager::AddComponent<WorldTransformComponent>(this->GetScene(), decalEnt);
                        
                        LOGGER_INFO("GameState") << "Spawned Decal at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
                    }
                }
            }
        }
    }

    // --- Demo Logic ---
    static std::shared_ptr<ISound> bgmSound = nullptr;

    // Toggle BGM
    if (input.GetActionDown("ToggleBGM")) {
        auto view = this->GetScene().registry.view<InfoComponent, AudioSourceComponent>();
        for (auto e : view) {
            auto& info = view.get<InfoComponent>(e);
            auto& audio = view.get<AudioSourceComponent>(e);
            if (info.name == "BGM_Player") {
                if (audio.sound) {
                    audio.sound->Stop();
                    audio.sound = nullptr;
                    audio.playOnAwake = false;
                } else {
                    audio.shouldPlay = true;
                }
                break;
            }
        }
    }

    // Toggle 2D Sound
    if (input.GetActionDown("Toggle2DSound")) {
        this->GetAudioService().GetEngine()->Play2D("resources/audios/2dsound.mp3", false);
    }

    // Volume Control
    if (input.GetActionDown("VolumeUp")) {
        AppConfig cfg = this->GetRuntimeCore().GetConfig();
        cfg.masterVolume = std::min(10.0f, cfg.masterVolume + 0.1f);
        this->GetRuntimeCore().ApplyConfig(cfg);
        LOGGER_INFO("GameState") << "Master Volume: " << cfg.masterVolume;
    }
    if (input.GetActionDown("VolumeDown")) {
        AppConfig cfg = this->GetRuntimeCore().GetConfig();
        cfg.masterVolume = std::max(0.0f, cfg.masterVolume - 0.1f);
        this->GetRuntimeCore().ApplyConfig(cfg);
        LOGGER_INFO("GameState") << "Master Volume: " << cfg.masterVolume;
    }

    // Toggle Particle
    if (input.GetActionDown("ToggleParticle")) {
        auto view = this->GetScene().registry.view<InfoComponent, ParticleEmitterComponent>();
        for (auto e : view) {
            auto& info = view.get<InfoComponent>(e);
            if (info.name == "Particle_Demo") {
                auto& emitter = view.get<ParticleEmitterComponent>(e);
                emitter.isActive = !emitter.isActive;
                LOGGER_INFO("GameState") << "Particle Emitter: " << (emitter.isActive ? "ON" : "OFF");
                break;
            }
        }
    }

    // Mouse Wheel Interactions
    float wheelMove = this->GetMouse().GetScrollY();
    if (std::abs(wheelMove) > 0.1f)
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(this->GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(this->GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(this->GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(this->GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = this->GetMouse().GetLastX();
            float mouseY = this->GetMouse().GetLastY();
            float screenW = (float)this->GetIOHandler().GetMonitorManager().GetWidth();
            float screenH = (float)this->GetIOHandler().GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / (screenW > 0 ? screenW : 1.0f) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / (screenH > 0 ? screenH : 1.0f);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = this->GetPhysicsSystem().GetPhysicsWorld().Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                if (wheelMove > 0) // WheelUp: 3D Sound
                {
                    this->GetAudioService().Play3D("resources/audios/3dsound.mp3", hit.hitPoint, false);
                    LOGGER_INFO("GameState") << "Play 3D Sound at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
                }
                else // WheelDown: Particle
                {
                    auto* info = EntityManager::TryGetComponent<InfoComponent>(this->GetScene(), hit.entity);
                    if (info && (info->name == "WallTest" || info->tag == "terrain"))
                    {
                        // Spawn a NEW "one-shot" impact particle
                        auto particleEnt = EntityManager::CreateEntity(this->GetScene(), "Impact_Particle");
                        
                        auto& pPos = EntityManager::AddComponent<PositionComponent>(this->GetScene(), particleEnt);
                        pPos.value = hit.hitPoint;
                        
                        auto& pComp = EntityManager::AddComponent<ParticleEmitterComponent>(this->GetScene(), particleEnt);
                        
                        // Configure for impact effect
                        pComp.isActive = true;
                        pComp.lifetime = 0.5f; // Spawning duration
                        pComp.emitter.Initialize(300);
                        pComp.emitter.Texture = this->GetResourceManager().GetTextureAuto("resources/textures/particle_star.png");
                        pComp.emitter.SpawnRate = 400.0f; 
                        pComp.emitter.LifeTime = 0.8f;   // Particles live slightly longer for fade
                        pComp.emitter.StartSize = 0.5f;
                        pComp.emitter.EndSize = 0.0f;
                        pComp.emitter.MinVelocity = glm::vec3(-3, 1, -3);
                        pComp.emitter.MaxVelocity = glm::vec3(3, 6, 3);
                        pComp.emitter.StartColor = glm::vec4(1.0f, 0.8f, 0.4f, 1.0f); // Sparkly orange/yellow
                        pComp.emitter.EndColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
                        pComp.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
                        
                        EntityManager::AddComponent<WorldTransformComponent>(this->GetScene(), particleEnt);
                        
                        LOGGER_INFO("GameState") << "Spawned Impact Particle at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
                    }
                }
            }
        }
    }

    // --- Shader Port Management ---
    auto demoView = this->GetScene().registry.view<InfoComponent, MaterialComponent>();
    for (auto entity : demoView)
    {
        auto [info, mat] = demoView.get<InfoComponent, MaterialComponent>(entity);
        
        // Port[0]: Team ID (1=Enemy/Red, 2=Ally/Blue)
        if (info.tag == "enemy") mat.desc.ports.data[0] = 1.0f;
        else if (info.tag == "ally") mat.desc.ports.data[0] = 2.0f;
        else mat.desc.ports.data[0] = 0.0f;

        // Port[1]: Selection Glow (1.0=On)
        mat.desc.ports.data[1] = (entity == m_SelectedEntity) ? 1.0f : 0.0f;
    }
}


void GameState::OnFixedUpdate(float fixedDt)
{
}

void GameState::OnRender()
{
}

void GameState::OnExit()
{
    m_SelectedEntity = entt::null;
    this->GetSceneManager().ClearAllScenes();
}
