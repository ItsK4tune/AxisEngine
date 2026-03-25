#include <ecs/logic/system_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/job_system.h>
#include <core/logic/event_system.h>
#include <core/logic/config_manager.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/interface/i_ui_service.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/geometry_system.h>
#include <ecs/logic/lighting_system.h>
#include <ecs/logic/shadow_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/ui_render_system.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/logic/decal_system.h>
#include <ecs/logic/transparent_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/transform_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/video_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/dummy_test_system.h>
#include <navigation/logic/navigation_system.h>
#include <render/logic/render_core.h>
#include <ecs/logic/streaming_system.h>
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
#include <ecs/logic/debug/debug_system.h>
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
        std::sort(m_RenderUISystems.begin(), m_RenderUISystems.end(), [](IRenderSystem* a, IRenderSystem* b) {
        return a->GetPriority() < b->GetPriority();
    });
    std::sort(m_RenderCaptureSystems.begin(), m_RenderCaptureSystems.end(), [](IRenderSystem* a, IRenderSystem* b) {
        return a->GetPriority() < b->GetPriority();
    });
    std::sort(m_PostProcessSystems.begin(), m_PostProcessSystems.end(), [](IRenderSystem* a, IRenderSystem* b) {
        return a->GetPriority() < b->GetPriority();
    });
}


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
    RegisterSystem(std::make_unique<ShadowSystem>());
    RegisterSystem(std::make_unique<GeometrySystem>());
    RegisterSystem(std::make_unique<LightingSystem>());
    RegisterSystem(std::make_unique<TransparentSystem>());
    RegisterSystem(std::make_unique<PostProcessSystem>());
    RegisterSystem(std::make_unique<NavigationSystem>());
    RegisterSystem(std::make_unique<DecalSystem>());
    RegisterSystem(std::make_unique<VideoSystem>());
    RegisterSystem(std::make_unique<StreamingSystem>());
    RegisterSystem(std::make_unique<TerrainSystem>());
    RegisterSystem(std::make_unique<DummyTestSystem>());
#ifdef ENABLE_DEBUG_SYSTEM
    RegisterSystem(std::make_unique<DebugSystem>());
#endif

    auto& sl = ServiceLocator::Instance();
    for (auto& sys : m_Systems) {
        sl.Register<IBaseSystem>(sys->GetName(), sys.get());
    }

    sl.Register<PhysicsSystem>(GetSystem<PhysicsSystem>());
    sl.Register<IRenderService>(GetSystem<RenderSystem>());
    sl.Register<AudioSystem>(GetSystem<AudioSystem>());
    sl.Register<UIRenderSystem>(GetSystem<UIRenderSystem>());
    sl.Register<ScriptableSystem>(GetSystem<ScriptableSystem>());
    sl.Register<NavigationSystem>(GetSystem<NavigationSystem>());
    sl.Register<IShadowService>(GetSystem<ShadowSystem>());
    sl.Register<IGeometryService>(GetSystem<GeometrySystem>());
    sl.Register<ILightingService>(GetSystem<LightingSystem>());
    sl.Register<ISkyboxService>(GetSystem<SkyboxRenderSystem>());
    sl.Register<IUIService>(GetSystem<UIRenderSystem>());

    auto& context = sl.Require<IGraphicsContext>();
    m_RenderCore = std::make_unique<RenderCore>();
    m_RenderCore->Initialize(context);
    sl.Register<RenderCore>(m_RenderCore.get());
}

void SystemManager::InitializeSystems(ResourceManager &res, int width, int height)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";

    m_UpdateSystems.clear();
    m_RenderSystems.clear();
    m_RenderAlphaSystems.clear();
    m_RenderTransparentSystems.clear();
    m_RenderMainSystems.clear();
    m_RenderUISystems.clear();
    m_RenderCaptureSystems.clear();
    m_PostProcessSystems.clear();
    
    for (auto& sys : m_Systems) {
        sys->Initialize();
        
        SystemCategory cat = sys->GetCategory();
        
        if ((cat & SystemCategory::Update) != SystemCategory::None) {
            if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sys.get())) {
                m_UpdateSystems.push_back(updateSys);
            }
        }

        if (auto* renderSys = dynamic_cast<IRenderSystem*>(sys.get())) {
            m_RenderSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderAlpha) != SystemCategory::None) { m_RenderAlphaSystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as RenderAlpha"; }
            if ((cat & SystemCategory::RenderMain) != SystemCategory::None) { m_RenderMainSystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as RenderMain"; }
            if ((cat & SystemCategory::RenderTransparent) != SystemCategory::None) { m_RenderTransparentSystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as RenderTransparent"; }
            if ((cat & SystemCategory::RenderUI) != SystemCategory::None) { m_RenderUISystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as RenderUI"; }
            if ((cat & SystemCategory::RenderCapture) != SystemCategory::None) { m_RenderCaptureSystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as RenderCapture"; }
            if ((cat & SystemCategory::PostProcess) != SystemCategory::None) { m_PostProcessSystems.push_back(renderSys); LOGGER_INFO("SystemManager") << "Registered " << sys->GetName() << " as PostProcess"; }
        }
    }

    auto sortRender = [](std::vector<IRenderSystem*>& systems) {
        std::sort(systems.begin(), systems.end(), [](IRenderSystem* a, IRenderSystem* b) {
            return a->GetPriority() < b->GetPriority();
        });
    };

    sortRender(m_RenderSystems);
    sortRender(m_RenderAlphaSystems);
    sortRender(m_RenderTransparentSystems);
    sortRender(m_RenderMainSystems);
    sortRender(m_RenderUISystems);
    sortRender(m_RenderCaptureSystems);
    sortRender(m_PostProcessSystems);

    RebuildExecutionBatches();
}

void SystemManager::Shutdown()
{
    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    for (auto& sys : m_Systems) {
        sys->Shutdown();
    }
    if (m_RenderCore) m_RenderCore->Shutdown();
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

    for (auto* sys : m_UpdateSystems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p < 30) {
            sys->Update(scene, dt);
        }
    }


    for (auto& batch : m_UpdateBatches) {
        if (batch.systems.empty()) continue;

        if (batch.systems.size() == 1) {
            if (batch.systems[0]->IsEnabled()) {
                batch.systems[0]->Update(scene, dt);
            }
        } else {
            JobSystem::JobCounter counter{0};
            for (auto* sys : batch.systems) {
                if (sys->IsEnabled()) {
                    JobSystem::Instance().Execute([sys, &scene, dt]() {
                        sys->Update(scene, dt);
                    }, &counter);
                }
            }
            JobSystem::Instance().Wait(&counter);
        }
    }


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
    auto ecsA = dynamic_cast<IECSSystem*>(a);
    auto readA = ecsA ? ecsA->GetReadComponents() : std::vector<entt::id_type>{};
    auto writeA = ecsA ? ecsA->GetWriteComponents() : std::vector<entt::id_type>{};
    auto ecsB = dynamic_cast<IECSSystem*>(b);
    auto readB = ecsB ? ecsB->GetReadComponents() : std::vector<entt::id_type>{};
    auto writeB = ecsB ? ecsB->GetWriteComponents() : std::vector<entt::id_type>{};

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

void SystemManager::RunRender(Scene &scene, int width, int height, float alpha) {
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto* rs = GetSystem<RenderSystem>();

    // 1. Capture Pass (Start PostProcess)
    for (IRenderSystem* sys : m_RenderCaptureSystems) {
        if (sys->IsEnabled()) sys->RenderCapturePass(scene, width, height);
    }

    // Update target for RenderSystem if PostProcess is active
    if (auto* pps = GetSystem<PostProcessSystem>()) {
        if (pps->IsEnabled() && rs) {
            rs->SetMainFBO(pps->GetCaptureFBO());
        }
    }

    // 2. Build Queues
    if (rs) rs->BuildRenderQueues(scene, alpha, width, height);

    // 3. Publish Render Data
    FrameRenderDataEvent ev;
    ev.data.mainFBO = rs ? rs->GetMainFBO() : 0;
    ev.data.width = width;
    ev.data.height = height;
    ev.data.alpha = alpha;
    EventSystem::Instance().Publish(ev);

    static bool firstFrame = true;
    // 4. Render Passes
    for (IRenderSystem* sys : m_RenderMainSystems) {
        if (sys->IsEnabled()) {
            if (firstFrame) LOGGER_INFO("SystemManager") << "Executing RenderMain: " << sys->GetName();
            sys->RenderAlphaPass(scene, width, height, alpha);
        }
    }
    
    for (IRenderSystem* sys : m_RenderTransparentSystems) {
        if (sys->IsEnabled()) {
            if (firstFrame) LOGGER_INFO("SystemManager") << "Executing RenderTransparent: " << sys->GetName();
            sys->RenderTransparentPass(scene, width, height, alpha);
        }
    }
    
    firstFrame = false;

    for (IRenderSystem* sys : m_RenderAlphaSystems) {
        if (sys->IsEnabled()) sys->Render(scene);
    }

    // 5. Flush
    if (rs) rs->FlushCommands();

    // 6. PostProcess (End Capture & Blit)
    for (IRenderSystem* sys : m_PostProcessSystems) {
        if (sys->IsEnabled()) sys->Render(scene);
    }

    // 7. UI Pass
    auto& rsm = context.GetRenderStateManager();
    for (IRenderSystem* sys : m_RenderUISystems) {
        if (sys->IsEnabled()) sys->RenderUIPass(scene, (float)width, (float)height, rsm);
    }
}

void SystemManager::UpdateDebugSystem(float realDeltaTime)
{
    if (auto* ds = GetSystem<DebugSystem>())
        ds->OnUpdate(realDeltaTime);
}

void SystemManager::RenderDebugSystem(Scene& scene)
{
    if (auto* ds = GetSystem<DebugSystem>())
        ds->Render(scene);
}

void SystemManager::RenderShadows(Scene& scene, float alpha)
{
    auto* rs = GetSystem<RenderSystem>();
    if (rs) rs->BuildRenderQueues(scene, alpha);

    auto* shadowSys = ServiceLocator::Instance().Resolve<IShadowService>();
    if (shadowSys && shadowSys->IsEnabled()) {
        shadowSys->Render(scene);
    }
}
