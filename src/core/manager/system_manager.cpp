#include <core/unit/engine_context.h>
#include <platform/logic/io_handler.h>
#include <core/manager/system_manager.h>
#include <audio/logic/sound_player.h>
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
    RegisterSystem(std::make_unique<DummyTestSystem>());

    for (auto& sys : m_Systems) {
        sys->Init(m_Ctx);
    }

    auto& context = ctx.io->GetGraphicsContext();
    postProcess.Init(context, width, height, res);
    
    GetSystem<RenderSystem>()->Init(context, res);
    GetSystem<SkyboxRenderSystem>()->Init(context);
    GetSystem<ParticleSystem>()->Init(context);

#ifdef ENABLE_DEBUG_SYSTEM
    m_DebugSystem = std::make_unique<DebugSystem>();
#else
    m_DebugSystem = std::make_unique<NullDebugSystem>();
#endif
    m_DebugSystem->Init(m_Ctx);
}


void SystemManager::Shutdown()
{
    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    for (auto& sys : m_Systems) {
        sys->Shutdown();
    }
    GetSystem<RenderSystem>()->Shutdown();
    postProcess.Shutdown();
}

void SystemManager::ApplyConfig(const AppConfig &config)
{
    auto* rs = GetSystem<RenderSystem>();
    if (rs) {
        rs->SetShadowMode(config.shadowMode);
        rs->SetShadowProjectionSize(config.shadowProjectionSize);
        rs->SetInstanceBatching(config.instanceBatchingEnabled);
        rs->SetFrustumCulling(config.frustumCullingEnabled);
        rs->SetOcclusionCulling(config.occlusionCullingEnabled);
        rs->SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
        rs->SetShadowDistanceCulling(config.shadowDistanceCulling);
        rs->SetDistanceCulling(config.distanceCulling);
        rs->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
    }
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
    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p < 30) {
            sys->Update(scene, dt);
        }
    }

    std::vector<std::future<void>> futures;
    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 30 && p < 80) {
            futures.push_back(JobSystem::Instance().ExecuteAsync([sys=sys.get(), &scene, dt]() {
                sys->Update(scene, dt);
            }));
        }
    }

    for (auto& f : futures) {
        f.get();
    }
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

    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 0) {
            if (p == 80 && rs) {
                rs->RenderAlpha(scene, width, height, alpha);
            } else if (p == 90 && rs) {
                if (auto* ui = GetSystem<UIRenderSystem>()) {
                    ui->RenderUI(scene, (float)width, (float)height, rs->GetContext()->GetRenderStateManager());
                }
            } else {
                sys->Render(scene);
            }
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
