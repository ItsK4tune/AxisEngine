#include <ecs/logic/deferred_lighting_system.h>
#include <physics/logic/physics_loader.h>
#include <platform/logic/io_handler.h>
#include <core/logic/system_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h> // Updated from <core/logic/event_types.h>
#include <core/type/app_config.h>
#include <core/logic/debug_system.h>
#include <audio/logic/audio_service.h>
#include <core/logic/runtime_core.h>
#include <core/logic/job_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/ui_render_system.h>
#include <ecs/logic/transform_system.h>
#include <ecs/logic/streaming_system.h>
#include <ecs/logic/video_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/decal_system.h>
#include <navigation/logic/navigation_system.h>
#include <ecs/logic/dummy_test_system.h>
#include <platform/logic/input_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <core/logic/logger.h>
#include <algorithm>
#ifdef ENABLE_DEBUG_SYSTEM
#include <core/logic/debug_system.h>
#else
#endif

SystemManager::SystemManager()
{
}

SystemManager::~SystemManager()
{
}

void SystemManager::RegisterSystem(std::unique_ptr<IBaseSystem> system)
{
    IBaseSystem* sysPtr = system.get();
    m_Systems.push_back(std::move(system));
    
    if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sysPtr)) {
        m_UpdateSystems.push_back(updateSys);
        std::sort(m_UpdateSystems.begin(), m_UpdateSystems.end(), [](IUpdateSystem* a, IUpdateSystem* b) {
            return a->GetPriority() < b->GetPriority();
        });
    }
    
    if (auto* renderSys = dynamic_cast<IRenderSystem*>(sysPtr)) {
        m_RenderSystems.push_back(renderSys);
        std::sort(m_RenderSystems.begin(), m_RenderSystems.end(), [](IRenderSystem* a, IRenderSystem* b) {
            return a->GetPriority() < b->GetPriority();
        });
    }

    // Still keep m_Systems sorted for GetSystem templates
    std::sort(m_Systems.begin(), m_Systems.end(), [](const std::unique_ptr<IBaseSystem>& a, const std::unique_ptr<IBaseSystem>& b) {
        return a->GetPriority() < b->GetPriority();
    });
}

IBaseSystem* SystemManager::GetSystem(const std::string& name) const
{
    for (const auto& sys : m_Systems)
    {
        if (sys->GetName() == name)
            return sys.get();
    }
    return nullptr;
}

void SystemManager::CreateSystems()
{
    if (!m_Systems.empty()) return;
    LOGGER_INFO("SystemManager") << "Creating systems...";

    RegisterSystem(std::make_unique<PhysicsSystem>());
    RegisterSystem(std::make_unique<RenderSystem>());
    RegisterSystem(std::make_unique<TransformSystem>());
    RegisterSystem(std::make_unique<AnimationSystem>());
    RegisterSystem(std::make_unique<UIRenderSystem>());
    RegisterSystem(std::make_unique<ScriptableSystem>());
    RegisterSystem(std::make_unique<SkyboxRenderSystem>());
    RegisterSystem(std::make_unique<AudioSystem>());
    RegisterSystem(std::make_unique<ParticleSystem>());
    RegisterSystem(std::make_unique<StreamingSystem>());
    RegisterSystem(std::make_unique<VideoSystem>());
    RegisterSystem(std::make_unique<NavigationSystem>());
    RegisterSystem(std::make_unique<TerrainSystem>());
    RegisterSystem(std::make_unique<DecalSystem>());
    RegisterSystem(std::make_unique<DeferredLightingSystem>());
    RegisterSystem(std::make_unique<DummyTestSystem>());

    // Register systems in ServiceLocator so they are available BEFORE initialization
    auto& sl = ServiceLocator::Instance();
    sl.Register<PhysicsSystem>(GetSystem<PhysicsSystem>());
    sl.Register<RenderSystem>(GetSystem<RenderSystem>());
    sl.Register<AudioSystem>(GetSystem<AudioSystem>());
    sl.Register<UIRenderSystem>(GetSystem<UIRenderSystem>());
    sl.Register<ScriptableSystem>(GetSystem<ScriptableSystem>());
    sl.Register<NavigationSystem>(GetSystem<NavigationSystem>());
}

void SystemManager::InitializeSystems(ResourceManager &res, int width, int height)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";

    for (auto& sys : m_Systems) {
        sys->Initialize();
    }

    RebuildExecutionBatches();

    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    
    auto& configMgr = sl.Require<ConfigManager>();
    postProcess.Initialize(context, width, height, res, configMgr.GetConfig());

#ifdef ENABLE_DEBUG_SYSTEM
    m_DebugSystem = std::make_unique<DebugSystem>();
#else
    m_DebugSystem = std::make_unique<NullDebugSystem>();
#endif
    m_DebugSystem->Initialize();
}

void SystemManager::Shutdown()
{
    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    for (auto& sys : m_Systems) {
        sys->Shutdown();
    }
    postProcess.Shutdown();
}



void SystemManager::RunFixedUpdate(Scene& scene, float fixedDt)
{
    for (auto* sys : m_UpdateSystems) {
        if (sys->IsEnabled() && sys->GetPriority() < 20) {
            sys->FixedUpdate(scene, fixedDt);
        }
    }
}

void SystemManager::RunUpdate(Scene& scene, float dt)
{
    // Serial update for p < 30
    for (auto* sys : m_UpdateSystems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p < 30) {
            sys->Update(scene, dt);
        }
    }

    // Parallel update in batches for p [30, 80)
    for (auto& batch : m_UpdateBatches) {
        if (batch.systems.empty()) continue;

        if (batch.systems.size() == 1) {
            if (batch.systems[0]->IsEnabled()) {
                batch.systems[0]->Update(scene, dt);
            }
        } else {
            std::vector<std::future<void>> futures;
            for (auto* sys : batch.systems) {
                if (sys->IsEnabled()) {
                    futures.push_back(JobSystem::Instance().ExecuteAsync([sys, &scene, dt]() {
                        sys->Update(scene, dt);
                    }));
                }
            }
            for (auto& f : futures) {
                f.get();
            }
        }
    }

    // Serial update for p >= 80
    for (auto* sys : m_UpdateSystems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 80) {
            sys->Update(scene, dt);
        }
    }
}

void SystemManager::RebuildExecutionBatches()
{
    m_UpdateBatches.clear();

    // Only batch systems with priority 30-80
    std::vector<IUpdateSystem*> systemsToBatch;
    for (auto* sys : m_UpdateSystems) {
        int p = sys->GetPriority();
        if (p >= 30 && p < 80) {
            systemsToBatch.push_back(sys);
        }
    }

    for (auto* sys : systemsToBatch) {
        bool added = false;
        for (auto& batch : m_UpdateBatches) {
            bool conflict = false;
            for (auto* batchSys : batch.systems) {
                if (SystemsConflict(sys, batchSys)) {
                    conflict = true;
                    break;
                }
            }

            if (!conflict) {
                batch.systems.push_back(sys);
                added = true;
                break;
            }
        }

        if (!added) {
            m_UpdateBatches.push_back({{sys}});
        }
    }

    LOGGER_INFO("SystemManager") << "Rebuilt execution batches. Total batches: " << m_UpdateBatches.size();
}

bool SystemManager::SystemsConflict(IUpdateSystem* a, IUpdateSystem* b) const
{
    auto readA = a->GetReadComponents();
    auto writeA = a->GetWriteComponents();
    auto readB = b->GetReadComponents();
    auto writeB = b->GetWriteComponents();

    for (auto id : writeA) {
        if (std::find(readB.begin(), readB.end(), id) != readB.end()) return true;
        if (std::find(writeB.begin(), writeB.end(), id) != writeB.end()) return true;
    }

    for (auto id : writeB) {
        if (std::find(readA.begin(), readA.end(), id) != readA.end()) return true;
        if (std::find(writeA.begin(), writeA.end(), id) != writeA.end()) return true;
    }

    return false;
}

void SystemManager::RenderShadows(Scene& scene, float alpha)
{
    if (auto* rs = GetSystem<RenderSystem>()) {
        rs->BuildRenderQueues(scene, alpha, 0, 0);
        rs->RenderShadows(scene);
    }
}

void SystemManager::RunRender(Scene& scene, int width, int height, float alpha)
{
    auto* rs = GetSystem<RenderSystem>();
    if (rs && rs->GetContext())
    {
        rs->GetContext()->GetRenderStateManager().SetViewport(0, 0, width, height);
    }
    
    postProcess.BeginCapture();
    if (rs) rs->SetMainFBO(postProcess.GetCaptureFBO());

    for (auto* sys : m_RenderSystems) { 
        if (sys->IsEnabled()) sys->RenderAlpha(scene, width, height, alpha); 
    }
    
    for (auto* sys : m_RenderSystems) { 
        if (sys->IsEnabled()) sys->RenderTransparent(scene, width, height, alpha); 
    }
    
    for (auto* sys : m_RenderSystems) { 
        if (sys->IsEnabled()) sys->Render(scene); 
    }
    
    for (auto* sys : m_RenderSystems) { 
        if (sys->IsEnabled() && rs && rs->GetContext()) 
            sys->RenderUI(scene, (float)width, (float)height, rs->GetContext()->GetRenderStateManager()); 
    }

    if (rs) {
        postProcess.ApplyAntiAliasing(rs->GetAntiAliasingMode(),
                                       rs->GetPrevViewProj(),
                                       rs->GetCurrViewProj(),
                                       rs->GetJitterOffset());
    }

    postProcess.EndCapture();
}

void SystemManager::UpdateDebugSystem(float realDeltaTime)
{
    if (m_DebugSystem)
        m_DebugSystem->OnUpdate(realDeltaTime);
}

void SystemManager::RenderDebugSystem(Scene& scene)
{
    if (m_DebugSystem)
        m_DebugSystem->Render(scene);
}
