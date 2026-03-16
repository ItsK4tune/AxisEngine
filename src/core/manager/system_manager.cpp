#include <ecs/logic/deferred_lighting_system.h>
#include <core/unit/engine_context.h>
#include <platform/logic/io_handler.h>
#include <core/manager/system_manager.h>
#include <core/logic/debug_core.h>
#include <audio/logic/sound_player.h>
#include <core/logic/engine_core.h>
#include <core/logic/job_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/script_system.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/logic/ui_system.h>
#include <ecs/logic/transform_system.h>
#include <ecs/logic/streaming_system.h>
#include <ecs/logic/video_system.h>
#include <ecs/logic/terrain_system.h>
#include <ecs/logic/decal_system.h>
#include <navigation/logic/navigation_system.h>
#include <ecs/logic/dummy_test_system.h>
#include <platform/logic/input_system.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <physics/interface/i_physics_world.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>
#include <core/logic/logger.h>
#include <algorithm>
#ifdef ENABLE_DEBUG_SYSTEM
#include <core/logic/debug_core.h>
#else
#endif

SystemManager::SystemManager()
{
}

SystemManager::~SystemManager()
{
}

void SystemManager::RegisterSystem(std::unique_ptr<ISystem> system)
{
    m_Systems.push_back(std::move(system));
    std::sort(m_Systems.begin(), m_Systems.end(), [](const std::unique_ptr<ISystem>& a, const std::unique_ptr<ISystem>& b) {
        return a->GetPriority() < b->GetPriority();
    });
}

ISystem* SystemManager::GetSystem(const std::string& name) const
{
    for (const auto& sys : m_Systems)
    {
        if (sys->GetName() == name)
            return sys.get();
    }
    return nullptr;
}

void SystemManager::InitializeSystems(ResourceManager& res, int width, int height, EngineContext ctx)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";

    m_Ctx = ctx;

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

    for (auto& sys : m_Systems) {
        sys->Initialize(m_Ctx);
    }

    RebuildExecutionBatches();

    auto& context = ctx.io->GetGraphicsContext();
    postProcess.Initialize(context, width, height, res, m_Ctx.runtime->GetConfig());
    
    auto* rs = GetSystem<RenderSystem>();
    if (rs)
        rs->Initialize(context, res, m_Ctx.runtime->GetConfig());
    GetSystem<SkyboxRenderSystem>()->Initialize(context);
    GetSystem<ParticleSystem>()->Initialize(context);
    GetSystem<DecalSystem>()->Initialize(context, res);

#ifdef ENABLE_DEBUG_SYSTEM
    m_DebugSystem = std::make_unique<DebugSystem>();
#else
    m_DebugSystem = std::make_unique<NullDebugSystem>();
#endif
    m_DebugSystem->Initialize(m_Ctx);
}

void SystemManager::Shutdown()
{
    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    for (auto& sys : m_Systems) {
        sys->Shutdown();
    }
    postProcess.Shutdown();
}

void SystemManager::ApplyConfig(const AppConfig &config)
{
    auto* rs = GetSystem<RenderSystem>();
    if (rs) {
        rs->SetEnableShadows(config.shadowsEnabled);
        rs->SetShadowMode(config.shadowMode);
        rs->SetShadowProjectionSize(config.shadowProjectionSize);
        rs->SetInstanceBatching(config.instanceBatchingEnabled);
        rs->SetFrustumCulling(config.frustumCullingEnabled);
        rs->SetOcclusionCulling(config.occlusionCullingEnabled);
        rs->SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
        rs->SetShadowDistanceCulling(config.shadowDistanceCulling);
        rs->SetDistanceCulling(config.distanceCulling);
        rs->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
        rs->SetShadowBias(config.shadowBias);
        rs->SetShadowSoftness(config.shadowSoftness);
        rs->SetRenderOrderEnabled(config.renderOrderEnabled);
        rs->SetFilterLayerMask(config.filterLayerMask);
        rs->SetDeferredRendering(config.renderPath == RenderPath::Deferred);
        
        rs->SetFogEnabled(config.fogEnabled);
        rs->SetFogColor(glm::vec3(config.fogColor[0], config.fogColor[1], config.fogColor[2]));
        rs->SetFogDensity(config.fogDensity);

        if (config.cullFaceEnabled)
            rs->SetFaceCulling(true);
        else
            rs->SetFaceCulling(false);

        if (config.depthTestEnabled)
            rs->SetDepthTest(true);
        else
            rs->SetDepthTest(false);
    }

    // Post-process pipeline
    postProcess.SetGamma(config.gamma);
    postProcess.SetExposure(config.exposure);
    postProcess.SetBloomIntensity(config.bloomIntensity);
    postProcess.SetBloomThreshold(config.bloomThreshold);
    postProcess.SetBloomRadius(config.bloomRadius);
    postProcess.SetSkyboxIntensity(config.skyboxIntensity);
    if (auto* sky = GetSystem<SkyboxRenderSystem>()) {
        static_cast<SkyboxRenderSystem*>(sky)->SetIntensity(config.skyboxIntensity);
    }
    postProcess.SetTonemappingMode(static_cast<int>(config.tonemappingMode));
    postProcess.SetHDREnabled(config.hdrEnabled);
    postProcess.SetBloomEnabled(config.bloomEnabled);
    postProcess.SetClearColor(config.clearColor[0], config.clearColor[1], config.clearColor[2], config.clearColor[3]);

    // Physics
    if (m_Ctx.IsValid() && m_Ctx.physics) {
        m_Ctx.physics->SetGravity(glm::vec3(config.gravity[0], config.gravity[1], config.gravity[2]));
        m_Ctx.physics->SetMode(static_cast<int>(config.physicsMode));
        m_Ctx.physics->SetSolverIterations(config.solverIterations);
        m_Ctx.physics->SetCCDEnabled(config.ccdEnabled, config.ccdThreshold);
    }

    Logger::SetLogLevel(config.logLevel);

    LOGGER_INFO("SystemManager") << "Applied engine configuration.";
}

void SystemManager::RunFixedUpdate(Scene& scene, float fixedDt)
{
    for (auto& sys : m_Systems) {
        if (sys->IsEnabled() && sys->GetPriority() < 20) {
            sys->FixedUpdate(scene, fixedDt);
        }
    }
}

void SystemManager::RunUpdate(Scene& scene, float dt)
{
    // Serial update for p < 30
    for (auto& sys : m_Systems) {
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

    // Serial update for p >= 80 (Note: RenderAlpha/RenderUI are called in RunRender)
    for (auto& sys : m_Systems) {
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
    std::vector<ISystem*> systemsToBatch;
    for (auto& sys : m_Systems) {
        int p = sys->GetPriority();
        if (p >= 30 && p < 80) {
            systemsToBatch.push_back(sys.get());
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

    // Log batches for verification
    LOGGER_INFO("SystemManager") << "Rebuilt execution batches. Total batches: " << m_UpdateBatches.size();
    for (size_t i = 0; i < m_UpdateBatches.size(); ++i) {
        std::string batchInfo = "Batch " + std::to_string(i) + ": ";
        for (auto* sys : m_UpdateBatches[i].systems) {
            batchInfo += sys->GetName() + " ";
        }
        LOGGER_INFO("SystemManager") << batchInfo;
    }
}

bool SystemManager::SystemsConflict(ISystem* a, ISystem* b) const
{
    auto readA = a->GetReadComponents();
    auto writeA = a->GetWriteComponents();
    auto readB = b->GetReadComponents();
    auto writeB = b->GetWriteComponents();

    // Check WriteA vs (ReadB U WriteB)
    for (auto id : writeA) {
        if (std::find(readB.begin(), readB.end(), id) != readB.end()) return true;
        if (std::find(writeB.begin(), writeB.end(), id) != writeB.end()) return true;
    }

    // Check WriteB vs (ReadA U WriteA)
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

    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        sys->RenderAlpha(scene, width, height, alpha);
    }

    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        if (rs) {
            sys->RenderUI(scene, (float)width, (float)height, rs->GetContext()->GetRenderStateManager());
        }
    }

    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        sys->RenderTransparent(scene, width, height, alpha);
    }
    
    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 0 && p != 80 && p != 90 && p != 100) {
            sys->Render(scene);
        }
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
