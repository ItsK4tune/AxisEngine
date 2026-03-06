#include <core/engine_context.h>
#include <window/io_handler.h>
#include <core/system_manager.h>
#include <audio/sound_player.h>
#include <core/job_system.h>
#include <ecs/systems/animation_system.h>
#include <ecs/systems/audio_system.h>
#include <ecs/systems/particle_system.h>
#include <ecs/systems/physics_system.h>
#include <ecs/systems/render_system.h>
#include <ecs/systems/script_system.h>
#include <ecs/systems/skybox_system.h>
#include <ecs/systems/ui_system.h>
#include <ecs/systems/video_system.h>
#include <ecs/systems/dummy_test_system.h>
#include <input/mouse_manager.h>
#include <graphics/interfaces/i_buffer_manager.h>
#include <graphics/interfaces/i_draw_context.h>
#include <graphics/interfaces/i_graphics_context.h>
#include <graphics/interfaces/i_render_state_manager.h>
#include <graphics/interfaces/i_render_target_manager.h>
#include <graphics/interfaces/i_shader_manager.h>
#include <graphics/interfaces/i_texture_manager.h>
#include <physics/interfaces/i_physics_world.h>
#include <resource/resource_manager.h>
#include <scene/scene.h>
#include <utils/logger.h>
#include <algorithm>
#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_system.h>
#else
#include <debug/null_debug_system.h>
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
    RegisterSystem(std::make_unique<AnimationSystem>());
    RegisterSystem(std::make_unique<UIRenderSystem>());
    RegisterSystem(std::make_unique<ScriptableSystem>());
    RegisterSystem(std::make_unique<SkyboxRenderSystem>());
    RegisterSystem(std::make_unique<AudioSystem>());
    RegisterSystem(std::make_unique<ParticleSystem>());
    m_Systems.push_back(std::make_unique<VideoSystem>());
    m_Systems.push_back(std::make_unique<DummyTestSystem>());

    for (auto& sys : m_Systems) {
        sys->Init(m_Ctx);
    }

    auto& context = ctx.io->GetGraphicsContext();
    postProcess.Init(context, width, height, res);
    
    // Explicit initializations
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
    // Specific shutdown
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
    // High priority (Logic)
    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 20 && p < 30) {
            sys->Update(scene, dt);
        }
    }

    // Medium priority (Visuals / Async)
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
        rs->GetContext()->GetRenderStateManager().Viewport(0, 0, width, height);
    }
    
    postProcess.BeginCapture();

    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        int p = sys->GetPriority();
        if (p >= 80) {
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
