#include <ecs/logic/system_manager.h>
#include <core/interface/i_optimization_configurable.h>
#include <audio/interface/i_audio_engine.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <core/interface/i_base_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_parallel_update_system.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/logic/system_factory.h>
#include <engine/platform/logic/io_handler.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_query_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <chrono>
#include <future>
#include <stdexcept>

namespace
{
using ProfileClock = std::chrono::steady_clock;

float ElapsedMs(ProfileClock::time_point start, ProfileClock::time_point end)
{
    return std::chrono::duration<float, std::milli>(end - start).count();
}

template <typename Fn>
void ProfilePass(ProfiledRenderPass pass, Fn&& fn)
{
    const auto start = ProfileClock::now();
    fn();
    RuntimeProfiler::Instance().AddPassTime(pass, ElapsedMs(start, ProfileClock::now()));
}

ProfiledRenderPass GetPassForSystem(const std::string& name)
{
    if (name == "GeometrySystem")
        return ProfiledRenderPass::Geometry;
    if (name == "LightingSystem")
        return ProfiledRenderPass::Lighting;
    if (name == "PostProcessSystem")
        return ProfiledRenderPass::PostProcess;
    return ProfiledRenderPass::Alpha;
}

ProfiledRenderPass GetUpdatePassForSystem(const std::string& name)
{
    if (name == "PhysicsSystem")
        return ProfiledRenderPass::Physics;
    if (name == "NavigationSystem")
        return ProfiledRenderPass::Navigation;
    return ProfiledRenderPass::Count;
}

void ProfileUpdateSystem(IUpdateSystem& system, Scene& scene, float dt)
{
    const ProfiledRenderPass pass = GetUpdatePassForSystem(system.GetName());
    if (pass == ProfiledRenderPass::Count)
    {
        system.Update(scene, dt);
        return;
    }
    ProfilePass(pass, [&]() { system.Update(scene, dt); });
}

}  // namespace

struct SystemManager::GpuFrameTimerState
{
    void Shutdown()
    {
        if (manager)
        {
            for (uint32_t query : queries)
                if (query != 0)
                    manager->DeleteQuery(query);
        }
        manager = nullptr;
        queries[0] = queries[1] = 0;
        submitted[0] = submitted[1] = false;
        currentIndex = 0;
        active = false;
    }

    void Begin(IQueryManager& queryManager)
    {
        if (!queryManager.SupportsQuery(QueryType::TimeElapsed))
            return;
        if (manager && manager != &queryManager)
            Shutdown();
        manager = &queryManager;

        if (queries[0] == 0)
        {
            queries[0] = manager->GenQuery();
            queries[1] = manager->GenQuery();
        }

        const int previous = 1 - currentIndex;
        if (submitted[previous] && manager->IsResultAvailable(queries[previous]))
        {
            const uint64_t elapsedNs = manager->GetQueryResult64(queries[previous]);
            RuntimeProfiler::Instance().SetGpuFrameTime(static_cast<float>(elapsedNs) / 1000000.0f);
            submitted[previous] = false;
        }

        manager->BeginQuery(QueryType::TimeElapsed, queries[currentIndex]);
        active = true;
    }

    void End()
    {
        if (!active || !manager)
            return;

        manager->EndQuery(QueryType::TimeElapsed);
        submitted[currentIndex] = true;
        currentIndex = 1 - currentIndex;
        active = false;
    }

    IQueryManager* manager = nullptr;
    uint32_t queries[2] = {0, 0};
    bool submitted[2] = {false, false};
    int currentIndex = 0;
    bool active = false;
};

namespace
{
template <typename Fn>
class ScopeExit
{
public:
    explicit ScopeExit(Fn fn) : m_Fn(std::move(fn))
    {
    }
    ~ScopeExit()
    {
        m_Fn();
    }

private:
    Fn m_Fn;
};
}  // namespace

SystemManager::SystemManager() : m_GpuFrameTimer(std::make_unique<GpuFrameTimerState>())
{
}

SystemManager::~SystemManager()
{
    Shutdown();
}

IBaseSystem* SystemManager::GetSystem(std::type_index concreteType) const
{
    const auto it = m_TypeCache.find(concreteType);
    return it != m_TypeCache.end() ? it->second : nullptr;
}

void SystemManager::RegisterSystem(std::unique_ptr<IBaseSystem> system)
{
    if (!system)
        throw std::invalid_argument("Cannot register a null system");
    if (!m_AcceptsRegistration)
        throw std::logic_error("Systems must be registered before SystemManager::Initialize");
    if (GetSystem(system->GetName()))
    {
        LOGGER_WARN("SystemManager") << "Ignoring duplicate system registration: " << system->GetName();
        return;
    }

    IBaseSystem* sysPtr = system.get();
    if (const auto collision = m_IdCache.find(system->GetId().value); collision != m_IdCache.end())
        throw std::invalid_argument("System id collision between " + collision->second->GetName() + " and " +
                                    system->GetName());
    m_Systems.push_back(std::move(system));

    // Auto-cache by concrete type
    m_TypeCache[std::type_index(typeid(*sysPtr))] = sysPtr;
    m_IdCache[sysPtr->GetId().value] = sysPtr;

    if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sysPtr))
    {
        m_UpdateSystems.push_back(updateSys);
    }

    if (auto* renderSys = dynamic_cast<IRenderSystem*>(sysPtr))
    {
        m_RenderSystems.push_back(renderSys);
    }

    if (auto* ecsSys = dynamic_cast<IECSSystem*>(sysPtr))
    {
        SystemAccessSet& access = m_SystemAccess[sysPtr];
        for (auto id : ecsSys->GetReadComponents())
        {
            access.read.insert(id);
        }
        for (auto id : ecsSys->GetWriteComponents())
        {
            access.write.insert(id);
        }
    }
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

IBaseSystem* SystemManager::GetSystem(SystemId id) const
{
    const auto it = m_IdCache.find(id.value);
    return it != m_IdCache.end() ? it->second : nullptr;
}

void SystemManager::CreateSystems()
{
    if (m_DefaultSystemsCreated)
        return;
    m_DefaultSystemsCreated = true;
    LOGGER_INFO("SystemManager") << "Creating systems (via Factory)...";

    // Use Factory to create all registered systems (OCP compliance)
    auto systems = SystemFactory::CreateAll();
    for (auto& sys : systems)
    {
        if (sys && !GetSystem(sys->GetName()))
            RegisterSystem(std::move(sys));
    }

    // Register all systems by name to ServiceLocator
    auto& sl = ServiceLocator::Instance();
    for (auto& sys : m_Systems)
    {
        sl.Register<IBaseSystem>(sys->GetName(), sys.get());
    }

    LOGGER_INFO("SystemManager") << "Created " << m_Systems.size() << " systems.";
}

void SystemManager::Initialize(ResourceManager& res, int width, int height)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";
    m_IsShutdown = false;
    m_AcceptsRegistration = false;
    m_EventSubscriptions.Clear();

    m_UpdateSystems.clear();
    m_RenderSystems.clear();
    m_RenderAlphaSystems.clear();
    m_RenderTransparentSystems.clear();
    m_RenderMainSystems.clear();
    m_RenderUISystems.clear();
    m_RenderCaptureSystems.clear();
    m_PostProcessSystems.clear();
    m_InitializedSystems.clear();

    // Subscribe to events
    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<SystemEnabledEvent>([this](const SystemEnabledEvent& e) {
            if (auto* sys = this->GetSystem(e.systemName))
            {
                if (sys->IsEnabled() != e.enabled)
                    sys->SetEnabled(e.enabled);
            }
        }));

    // Sort systems by priority before initialization if needed,
    // but usually RegisterSystem and category flags are enough.

    // Build capability bitmask from available services
    auto& sl = ServiceLocator::Instance();
    m_AvailableCapabilities = 0;
    if (sl.Has<IGraphicsContext>())
        m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Graphics);
    if (sl.Has<IOHandler>())
        m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Input);

    // Audio check: only available if IAudioEngine service is registered
    if (sl.Has<IAudioEngine>())
    {
        m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Audio);
    }

    for (auto& sys : m_Systems)
    {
        uint32_t required = static_cast<uint32_t>(sys->GetRequirements());
        uint32_t unmet = required & ~m_AvailableCapabilities;

        if (unmet != 0)
        {
            LOGGER_WARN("SystemManager") << "Disabling system with unmet capabilities (" << unmet
                                         << "): " << sys->GetName();
            sys->SetEnabled(false);
            continue;
        }

        sys->Initialize();
        m_InitializedSystems.push_back(sys.get());

        SystemCategory cat = sys->GetCategory();
        LOGGER_INFO("SystemManager") << "Sys: " << sys->GetName() << " | Cat: " << (int)cat
                                     << " | Prio: " << sys->GetPriority();

        if ((cat & SystemCategory::Update) != SystemCategory::None)
        {
            if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sys.get()))
            {
                m_UpdateSystems.push_back(updateSys);
            }
        }

        if (auto* renderSys = dynamic_cast<IRenderSystem*>(sys.get()))
        {
            m_RenderSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderAlpha) != SystemCategory::None)
                m_RenderAlphaSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderMain) != SystemCategory::None)
                m_RenderMainSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderTransparent) != SystemCategory::None)
                m_RenderTransparentSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderUI) != SystemCategory::None)
                m_RenderUISystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderCapture) != SystemCategory::None)
                m_RenderCaptureSystems.push_back(renderSys);
            if ((cat & SystemCategory::PostProcess) != SystemCategory::None)
                m_PostProcessSystems.push_back(renderSys);
        }
    }

    std::sort(m_UpdateSystems.begin(), m_UpdateSystems.end(),
              [](IUpdateSystem* left, IUpdateSystem* right) { return left->GetPriority() < right->GetPriority(); });

    auto sortRender = [](std::vector<IRenderSystem*>& systems) {
        if (systems.empty())
            return;  // Fix for sorting empty lists
        std::sort(systems.begin(), systems.end(),
                  [](IRenderSystem* a, IRenderSystem* b) { return a->GetPriority() < b->GetPriority(); });
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
    if (m_IsShutdown)
        return;

    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    m_EventSubscriptions.Clear();
    for (auto it = m_InitializedSystems.rbegin(); it != m_InitializedSystems.rend(); ++it)
    {
        (*it)->Shutdown();
    }
    m_InitializedSystems.clear();
    if (m_GpuFrameTimer)
        m_GpuFrameTimer->Shutdown();
    m_IsShutdown = true;
}

void SystemManager::Reset()
{
    for (auto* system : m_InitializedSystems) system->Reset();
}

void SystemManager::FixedUpdate(Scene& scene, float fixedDt)
{
    for (auto* sys : m_UpdateSystems)
    {
        if (sys->IsEnabled() && sys->WantsFixedUpdate())
        {
            const ProfiledRenderPass pass = GetUpdatePassForSystem(sys->GetName());
            if (pass == ProfiledRenderPass::Count)
                sys->FixedUpdate(scene, fixedDt);
            else
                ProfilePass(pass, [&]() { sys->FixedUpdate(scene, fixedDt); });
        }
    }
}

void SystemManager::Update(Scene& scene, float dt)
{
    // 1. Early Update (Priority < 30)
    for (auto* sys : m_UpdateSystems)
    {
        if (!sys->IsEnabled())
            continue;
        if (sys->GetPriority() < 30)
        {
            ProfileUpdateSystem(*sys, scene, dt);
        }
    }

    // 2. Mid update. Only snapshot-aware systems can leave the main thread.
    for (auto& batch : m_UpdateBatches)
    {
        if (batch.systems.empty())
            continue;

        m_ParallelSystemsScratch.clear();

        auto flushParallel = [&]() {
            if (m_ParallelSystemsScratch.empty())
                return;

            m_FrameSnapshotsScratch.resize(m_ParallelSystemsScratch.size());
            m_CommandBuffersScratch.resize(m_ParallelSystemsScratch.size());

            for (size_t i = 0; i < m_ParallelSystemsScratch.size(); ++i)
            {
                m_FrameSnapshotsScratch[i].Clear();
                m_CommandBuffersScratch[i].Clear();
                m_ParallelSystemsScratch[i]->CaptureSnapshot(scene, m_FrameSnapshotsScratch[i]);
            }

            if (m_ParallelSystemsScratch.size() == 1)
            {
                m_ParallelSystemsScratch[0]->UpdateParallel(m_FrameSnapshotsScratch[0], m_CommandBuffersScratch[0],
                                                            dt);
            }
            else
            {
                m_UpdateFuturesScratch.clear();
                m_UpdateFuturesScratch.reserve(m_ParallelSystemsScratch.size());
                for (size_t i = 0; i < m_ParallelSystemsScratch.size(); ++i)
                {
                    m_UpdateFuturesScratch.push_back(JobSystem::Instance().ExecuteAsync([&, i]() {
                        m_ParallelSystemsScratch[i]->UpdateParallel(m_FrameSnapshotsScratch[i],
                                                                    m_CommandBuffersScratch[i], dt);
                    }));
                }

                for (auto& future : m_UpdateFuturesScratch)
                {
                    future.get();
                }
                m_UpdateFuturesScratch.clear();
            }

            for (size_t i = 0; i < m_ParallelSystemsScratch.size(); ++i)
            {
                m_CommandBuffersScratch[i].Apply(scene);
                m_FrameSnapshotsScratch[i].Clear();
            }

            m_ParallelSystemsScratch.clear();
        };

        for (auto* sys : batch.systems)
        {
            if (!sys->IsEnabled())
                continue;

            if (auto* parallelSys = dynamic_cast<IParallelUpdateSystem*>(sys))
            {
                m_ParallelSystemsScratch.push_back(parallelSys);
                continue;
            }

            flushParallel();
            ProfileUpdateSystem(*sys, scene, dt);
        }

        flushParallel();
    }

    // 3. Late Update (Priority >= 80)
    for (auto* sys : m_UpdateSystems)
    {
        if (!sys->IsEnabled())
            continue;
        if (sys->GetPriority() >= 80)
        {
            ProfileUpdateSystem(*sys, scene, dt);
        }
    }
}

void SystemManager::RebuildExecutionBatches()
{
    m_UpdateBatches.clear();

    std::vector<IUpdateSystem*> systemsToBatch;
    for (auto* sys : m_UpdateSystems)
    {
        int p = sys->GetPriority();
        if (p >= 30 && p < 80)
        {
            systemsToBatch.push_back(sys);
        }
    }

    for (auto* sys : systemsToBatch)
    {
        bool added = false;
        for (auto& batch : m_UpdateBatches)
        {
            if (!batch.systems.empty() && batch.systems.front()->GetPriority() != sys->GetPriority())
                continue;
            bool conflict = false;
            for (auto* batchSys : batch.systems)
            {
                if (SystemsConflict(sys, batchSys))
                {
                    conflict = true;
                    break;
                }
            }

            if (!conflict)
            {
                batch.systems.push_back(sys);
                added = true;
                break;
            }
        }

        if (!added)
        {
            m_UpdateBatches.push_back({{sys}});
        }
    }

    // Safety Audit: Verify compiled batches do not contain internal conflicts
    for (size_t i = 0; i < m_UpdateBatches.size(); ++i)
    {
        const auto& batch = m_UpdateBatches[i];
        for (size_t j = 0; j < batch.systems.size(); ++j)
        {
            for (size_t k = j + 1; k < batch.systems.size(); ++k)
            {
                if (SystemsConflict(batch.systems[j], batch.systems[k]))
                {
                    LOGGER_ERROR("SystemManager")
                        << "ECS Safety Violation: " << batch.systems[j]->GetName() << " and "
                        << batch.systems[k]->GetName() << " conflict in the same parallel batch!";
                    throw std::runtime_error(
                        "SystemManager: Parallel batch contains conflicting ECS queries! Possible Data Race.");
                }
            }
        }
    }

    LOGGER_INFO("SystemManager") << "Rebuilt execution batches. Total batches: " << m_UpdateBatches.size();
}

bool SystemManager::SystemsConflict(IUpdateSystem* a, IUpdateSystem* b) const
{
    auto itA = m_SystemAccess.find(dynamic_cast<IBaseSystem*>(a));
    auto itB = m_SystemAccess.find(dynamic_cast<IBaseSystem*>(b));

    if (itA == m_SystemAccess.end() || itB == m_SystemAccess.end())
        return false;

    const auto intersects = [](const auto& left, const auto& right) {
        const auto* smaller = &left;
        const auto* larger = &right;
        if (smaller->size() > larger->size())
            std::swap(smaller, larger);
        return std::any_of(smaller->begin(), smaller->end(), [&](entt::id_type id) { return larger->contains(id); });
    };

    const auto& accessA = itA->second;
    const auto& accessB = itB->second;
    return intersects(accessA.write, accessB.read) || intersects(accessB.write, accessA.read) ||
           intersects(accessA.write, accessB.write);
}

void SystemManager::Render(Scene& scene, int width, int height, float alpha)
{
    auto& sl = ServiceLocator::Instance();
    auto* graphicsContext = sl.Resolve<IGraphicsContext>();
    if (!graphicsContext)
        return;

    auto& profiler = RuntimeProfiler::Instance();
    m_GpuFrameTimer->Begin(graphicsContext->GetQueryManager());
    ScopeExit gpuTimer([this] { m_GpuFrameTimer->End(); });

    // 1. Shadow Pass
    ProfilePass(ProfiledRenderPass::Shadow, [&]() { RenderShadows(scene, width, height, alpha); });

    // 2. Capture Pass (Optional)
    ProfilePass(ProfiledRenderPass::Capture, [&]() {
        for (IRenderSystem* sys : m_RenderCaptureSystems)
        {
            if (sys->IsEnabled() || sys->GetName() == "PostProcessSystem")
            {
                sys->RenderCapturePass(scene, width, height);
            }
        }
    });

    // 3. Main Render Pass
    for (IRenderSystem* sys : m_RenderMainSystems)
    {
        if (sys->IsEnabled() || sys->GetName() == "PostProcessSystem")
        {
            ProfilePass(GetPassForSystem(sys->GetName()), [&]() { sys->Render(scene); });
        }
    }

    // 4. Alpha Pass
    for (IRenderSystem* sys : m_RenderAlphaSystems)
    {
        if (sys->IsEnabled())
            ProfilePass(ProfiledRenderPass::Alpha, [&]() { sys->RenderAlphaPass(scene, width, height, alpha); });
    }

    if (auto* rs = sl.Resolve<IRenderService>())
    {
        rs->RenderOcclusionQueries(scene, alpha);
    }

    // 5. Transparent Pass
    for (IRenderSystem* sys : m_RenderTransparentSystems)
    {
        if (sys->IsEnabled())
        {
            ProfilePass(ProfiledRenderPass::Transparent,
                        [&]() { sys->RenderTransparentPass(scene, width, height, alpha); });
        }
    }

    // 6. PostProcess
    for (IRenderSystem* sys : m_PostProcessSystems)
    {
        if (sys->IsEnabled() || sys->GetName() == "PostProcessSystem")
            ProfilePass(ProfiledRenderPass::PostProcess, [&]() { sys->Render(scene); });
    }

    // 7. UI Pass
    IRenderStateManager* rsm = &graphicsContext->GetRenderStateManager();
    for (IRenderSystem* sys : m_RenderUISystems)
    {
        if (sys->IsEnabled() || sys->GetName() == "PostProcessSystem")
        {
            ProfilePass(ProfiledRenderPass::UI, [&]() { sys->RenderUIPass(scene, (float)width, (float)height, *rsm); });
        }
    }

}

void SystemManager::RenderDebug(Scene& scene)
{
    for (auto& sys : m_Systems)
    {
        if (!sys->IsEnabled())
            continue;

        if ((sys->GetCategory() & SystemCategory::EditorOverlay) != SystemCategory::None)
        {
            auto* context = ServiceLocator::Instance().Resolve<IGraphicsContext>();
            if (context)
            {
                auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
                if (io)
                {
                    int w = io->GetMonitorManager().GetWidth();
                    int h = io->GetMonitorManager().GetHeight();
                    auto& rsm = context->GetRenderStateManager();

                    if (auto* renderSys = dynamic_cast<IRenderSystem*>(sys.get()))
                    {
                        renderSys->RenderUIPass(scene, (float)w, (float)h, rsm);
                    }
                }
            }
        }
    }
}

void SystemManager::RenderShadows(Scene& scene, int width, int height, float alpha)
{
    auto& sl = ServiceLocator::Instance();
    auto* rs = sl.Resolve<IRenderService>();
    if (rs)
        rs->BuildRenderQueues(scene, alpha, width, height);

    auto* shadowSys = sl.Resolve<IShadowService>();
    if (shadowSys && shadowSys->IsEnabled())
    {
        shadowSys->Render(scene);
    }
}
void SystemManager::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    for (const auto& system : m_Systems)
    {
        if (auto* configurable = dynamic_cast<IOptimizationConfigurable*>(system.get()))
            configurable->ApplyOptimizationConfig(config);
    }
}
