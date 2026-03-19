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

void GameState::OnEnter()
{
    LoadScene("scenes/game3.axs");
    LoadScene("scenes/ui.axs");
    SetCursorMode(CursorMode::Normal);
    LoadInputBindings("resources/configs/binding.axs");

    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);

    GetRenderSystem().SetFilterLayerMask(1);
}

void GameState::OnUpdate(float dt)
{
    auto& input = GetInputManager();

    if (input.GetActionDown("LoadNextScene")) QueueLoadScene("scenes/game2.axs");
    if (input.GetActionDown("ReloadScene")) QueuePopScene();
    if (input.GetActionDown("Pause")) GetRuntimeCore().GetStateMachine().PushState(std::make_unique<PauseState>());

    // Selection & Movement Logic (Combined into Mouse Left Click)
    if (input.GetActionDown("Select")) 
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = GetMouse().GetLastX();
            float mouseY = GetMouse().GetLastY();
            float screenW = (float)GetIOHandler().GetMonitorManager().GetWidth();
            float screenH = (float)GetIOHandler().GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / (screenW > 0 ? screenW : 1.0f) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / (screenH > 0 ? screenH : 1.0f);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = GetPhysicsSystem().GetPhysicsWorld().Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                auto* info = EntityManager::TryGetComponent<InfoComponent>(GetScene(), hit.entity);
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
                        if (auto* follower = EntityManager::TryGetComponent<PathFollowerComponent>(GetScene(), m_SelectedEntity))
                        {
                            follower->lockXPitch = true;
                            follower->lockZRoll = true;
                            GetNavigationSystem().MoveTo(GetScene(), m_SelectedEntity, hit.hitPoint);
                            LOGGER_INFO("GameState") << "Move Command: target=(" << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z << ") entity=" << (uint32_t)m_SelectedEntity;
                        }
                    }
                    
                    // ALWAYS spawn decal on click if we hit something (terrain, etc)
                    auto* decalSys = ServiceLocator::Instance().Require<SystemManager>().GetSystem<DecalSystem>();
                    if (decalSys)
                    {
                        static uint32_t crackTex = 0;
                        if (crackTex == 0) crackTex = decalSys->LoadDecalTexture("resources/textures/decal_crack.png");
                        
                        auto decalEnt = EntityManager::CreateEntity(GetScene(), "Decal_Crack");
                        auto& dComp = EntityManager::AddComponent<DecalComponent>(GetScene(), decalEnt);
                        dComp.albedoMap = crackTex;
                        dComp.lifetime = 10.0f; // Fade out after 10s
                        dComp.targetTags = {"terrain", "wall"}; // API demo: stick to terrain and wall
                        
                        auto& dPos = EntityManager::AddComponent<PositionComponent>(GetScene(), decalEnt);
                        dPos.value = hit.hitPoint;
                        
                        auto& dRot = EntityManager::AddComponent<RotationComponent>(GetScene(), decalEnt);
                        // Align decal box to surface normal
                        glm::vec3 up = glm::vec3(0, 1, 0);
                        if (glm::abs(glm::dot(hit.hitNormal, up)) > 0.99f) up = glm::vec3(1, 0, 0);
                        dRot.value = glm::quatLookAt(-hit.hitNormal, up); // Decal projects along -Z
                        
                        auto& dScale = EntityManager::AddComponent<ScaleComponent>(GetScene(), decalEnt);
                        dScale.value = glm::vec3(1.0f, 1.0f, 1.0f);
                        
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
        auto view = GetScene().registry.view<InfoComponent, AudioSourceComponent>();
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
        GetAudioService().GetEngine()->Play2D("resources/audios/2dsound.mp3", false);
    }

    // Volume Control
    if (input.GetActionDown("VolumeUp")) {
        AppConfig cfg = GetRuntimeCore().GetConfig();
        cfg.masterVolume = std::min(10.0f, cfg.masterVolume + 0.1f);
        GetRuntimeCore().ApplyConfig(cfg);
        LOGGER_INFO("GameState") << "Master Volume: " << cfg.masterVolume;
    }
    if (input.GetActionDown("VolumeDown")) {
        AppConfig cfg = GetRuntimeCore().GetConfig();
        cfg.masterVolume = std::max(0.0f, cfg.masterVolume - 0.1f);
        GetRuntimeCore().ApplyConfig(cfg);
        LOGGER_INFO("GameState") << "Master Volume: " << cfg.masterVolume;
    }

    // Toggle Particle
    if (input.GetActionDown("ToggleParticle")) {
        auto view = GetScene().registry.view<InfoComponent, ParticleEmitterComponent>();
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
    float wheelMove = GetMouse().GetScrollY();
    if (std::abs(wheelMove) > 0.1f)
    {
        entt::entity camEntity = EntityManager::GetActiveCamera(GetScene());
        if (camEntity != entt::null)
        {
            auto& camComp = EntityManager::GetComponent<CameraComponent>(GetScene(), camEntity);
            glm::vec3 camPos = (EntityManager::TryGetComponent<PositionComponent>(GetScene(), camEntity)) ? 
                                EntityManager::GetComponent<PositionComponent>(GetScene(), camEntity).value : glm::vec3(0.0f);
            
            float mouseX = GetMouse().GetLastX();
            float mouseY = GetMouse().GetLastY();
            float screenW = (float)GetIOHandler().GetMonitorManager().GetWidth();
            float screenH = (float)GetIOHandler().GetMonitorManager().GetHeight();
            
            float x = (2.0f * mouseX) / (screenW > 0 ? screenW : 1.0f) - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / (screenH > 0 ? screenH : 1.0f);
            
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
            glm::vec4 rayEye = glm::inverse(camComp.projectionMatrix) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(camComp.viewMatrix) * rayEye));
            
            RayHit hit = GetPhysicsSystem().GetPhysicsWorld().Raycast(camPos, rayDir, 1000.0f);
            if (hit.hasHit)
            {
                if (wheelMove > 0) // WheelUp: 3D Sound
                {
                    GetAudioService().Play3D("resources/audios/3dsound.mp3", hit.hitPoint, false);
                    LOGGER_INFO("GameState") << "Play 3D Sound at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
                }
                else // WheelDown: Particle
                {
                    auto* info = EntityManager::TryGetComponent<InfoComponent>(GetScene(), hit.entity);
                    if (info && (info->name == "WallTest" || info->tag == "terrain"))
                    {
                        // Spawn a NEW "one-shot" impact particle
                        auto particleEnt = EntityManager::CreateEntity(GetScene(), "Impact_Particle");
                        
                        auto& pPos = EntityManager::AddComponent<PositionComponent>(GetScene(), particleEnt);
                        pPos.value = hit.hitPoint;
                        
                        auto& pComp = EntityManager::AddComponent<ParticleEmitterComponent>(GetScene(), particleEnt);
                        
                        // Configure for impact effect
                        pComp.isActive = true;
                        pComp.lifetime = 0.5f; // Spawning duration
                        pComp.emitter.Initialize(300);
                        pComp.emitter.Texture = GetResourceManager().GetTextureAuto("resources/textures/particle_star.png");
                        pComp.emitter.SpawnRate = 400.0f; 
                        pComp.emitter.LifeTime = 0.8f;   // Particles live slightly longer for fade
                        pComp.emitter.StartSize = 0.5f;
                        pComp.emitter.EndSize = 0.0f;
                        pComp.emitter.MinVelocity = glm::vec3(-3, 1, -3);
                        pComp.emitter.MaxVelocity = glm::vec3(3, 6, 3);
                        pComp.emitter.StartColor = glm::vec4(1.0f, 0.8f, 0.4f, 1.0f); // Sparkly orange/yellow
                        pComp.emitter.EndColor = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
                        pComp.emitter.Shape = ParticleEmitter::EmissionShape::CONE;
                        
                        LOGGER_INFO("GameState") << "Spawned Impact Particle at " << hit.hitPoint.x << "," << hit.hitPoint.y << "," << hit.hitPoint.z;
                    }
                }
            }
        }
    }

    // --- Shader Port Management ---
    auto demoView = GetScene().registry.view<InfoComponent, MaterialComponent>();
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
    GetSceneManager().ClearAllScenes();
}
