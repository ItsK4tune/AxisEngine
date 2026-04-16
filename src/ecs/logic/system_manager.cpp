#include <ecs/logic/system_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/job_system.h>
#include <core/logic/event_manager.h>
#include <core/logic/config_manager.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_base_system.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/interface/i_ui_service.h>
#include <ecs/interface/i_ecs_system.h>
#include <render/interface/i_graphics_context.h>
#include <ecs/logic/system_factory.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <core/logic/logger.h>
#include <engine/platform/logic/io_handler.h>
#include <engine/audio/logic/audio_service.h>
#include <algorithm>

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
    
    // Auto-cache by concrete type
    m_TypeCache[std::type_index(typeid(*sysPtr))] = sysPtr;

    if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sysPtr)) {
        m_UpdateSystems.push_back(updateSys);
    }
    
    if (auto* renderSys = dynamic_cast<IRenderSystem*>(sysPtr)) {
        m_RenderSystems.push_back(renderSys);
    }

    // Bitset optimization for conflict detection (Layer 4)
    if (auto* ecsSys = dynamic_cast<IECSSystem*>(sysPtr)) {
        SystemBitset& bs = m_SystemBitsets[sysPtr];
        for (auto id : ecsSys->GetReadComponents()) {
            bs.read.set(GetComponentBitIndex(id) % 128);
        }
        for (auto id : ecsSys->GetWriteComponents()) {
            bs.write.set(GetComponentBitIndex(id) % 128);
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

void SystemManager::CreateSystems()
{
    if (!m_Systems.empty()) return;
    LOGGER_INFO("SystemManager") << "Creating systems (via Factory)...";

    // Use Factory to create all registered systems (OCP compliance)
    auto systems = SystemFactory::CreateAll();
    for (auto& sys : systems) {
        RegisterSystem(std::move(sys));
    }

    // Register all systems by name to ServiceLocator
    auto& sl = ServiceLocator::Instance();
    for (auto& sys : m_Systems) {
        sl.Register<IBaseSystem>(sys->GetName(), sys.get());
    }

    LOGGER_INFO("SystemManager") << "Created " << m_Systems.size() << " systems.";
}

void SystemManager::Initialize(ResourceManager &res, int width, int height)
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
    
    // Subscribe to events
    EventManager::Instance().Subscribe<SystemEnabledEvent>([this](const SystemEnabledEvent& e) {
        if (auto* sys = this->GetSystem(e.systemName)) {
            sys->SetEnabled(e.enabled);
        }
    });

    // Sort systems by priority before initialization if needed, 
    // but usually RegisterSystem and category flags are enough.
    
    // Build capability bitmask from available services
    auto& sl = ServiceLocator::Instance();
    m_AvailableCapabilities = 0;
    if (sl.Has<IGraphicsContext>()) m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Graphics);
    if (sl.Has<IOHandler>())       m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Input);
    
    // Audio check: only available if service exists and engine is valid (dummy removal handles rest)
    auto* audioSvc = sl.Resolve<AudioService>();
    if (audioSvc && audioSvc->GetEngine()) {
        m_AvailableCapabilities |= static_cast<uint32_t>(SystemRequirement::Audio);
    }

    for (auto& sys : m_Systems) {
        uint32_t required = static_cast<uint32_t>(sys->GetRequirements());
        uint32_t unmet = required & ~m_AvailableCapabilities;
        
        if (unmet != 0) {
            // Only force-disable in headless mode or if absolutely necessary.
            // For now, we only alert but allow the system to try initializing anyway if not headless.
            LOGGER_INFO("SystemManager") << "System requirements partially unmet (" << unmet << "): " << sys->GetName();
            
            if (sl.Require<ConfigManager>().IsHeadless()) {
                LOGGER_WARN("SystemManager") << "Disabling system due to headless mode: " << sys->GetName();
                sys->SetEnabled(false);
                continue;
            }
        }

        sys->Initialize();
        
        SystemCategory cat = sys->GetCategory();
        LOGGER_INFO("SystemManager") << "Sys: " << sys->GetName() << " | Cat: " << (int)cat << " | Prio: " << sys->GetPriority();
        
        if ((cat & SystemCategory::Update) != SystemCategory::None) {
            if (auto* updateSys = dynamic_cast<IUpdateSystem*>(sys.get())) {
                m_UpdateSystems.push_back(updateSys);
            }
        }

        if (auto* renderSys = dynamic_cast<IRenderSystem*>(sys.get())) {
            m_RenderSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderAlpha) != SystemCategory::None) m_RenderAlphaSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderMain) != SystemCategory::None) m_RenderMainSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderTransparent) != SystemCategory::None) m_RenderTransparentSystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderUI) != SystemCategory::None) m_RenderUISystems.push_back(renderSys);
            if ((cat & SystemCategory::RenderCapture) != SystemCategory::None) m_RenderCaptureSystems.push_back(renderSys);
            if ((cat & SystemCategory::PostProcess) != SystemCategory::None) m_PostProcessSystems.push_back(renderSys);
        }
    }

    auto sortRender = [](std::vector<IRenderSystem*>& systems) {
        if (systems.empty()) return; // Fix for sorting empty lists
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
}

void SystemManager::FixedUpdate(Scene& scene, float fixedDt)
{
    for (auto* sys : m_UpdateSystems) {
        if (sys->IsEnabled() && sys->GetPriority() < 20) {
            sys->FixedUpdate(scene, fixedDt);
        }
    }
}

void SystemManager::Update(Scene& scene, float dt)
{
    // 1. Early Update (Priority < 30)
    for (auto* sys : m_UpdateSystems) {
        if (!sys->IsEnabled()) continue;
        if (sys->GetPriority() < 30) {
            sys->Update(scene, dt);
        }
    }

    // 2. Batched Parallel Update (Priority 30-79)
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

    // 3. Late Update (Priority >= 80)
    for (auto* sys : m_UpdateSystems) {
        if (!sys->IsEnabled()) continue;
        if (sys->GetPriority() >= 80) {
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
    auto itA = m_SystemBitsets.find(dynamic_cast<IBaseSystem*>(a));
    auto itB = m_SystemBitsets.find(dynamic_cast<IBaseSystem*>(b));
    
    if (itA == m_SystemBitsets.end() || itB == m_SystemBitsets.end()) return false;

    const auto& bsA = itA->second;
    const auto& bsB = itB->second;

    // A write conflicts with B read
    if ((bsA.write & bsB.read).any()) return true;
    // B write conflicts with A read
    if ((bsB.write & bsA.read).any()) return true;
    // A write conflicts with B write (Race condition)
    if ((bsA.write & bsB.write).any()) return true;

    return false;
}

uint32_t SystemManager::GetComponentBitIndex(entt::id_type id)
{
    static std::unordered_map<entt::id_type, uint32_t> s_BitMapping;
    auto it = s_BitMapping.find(id);
    if (it != s_BitMapping.end()) return it->second;
    
    uint32_t index = (uint32_t)s_BitMapping.size();
    s_BitMapping[id] = index;
    return index;
}

void SystemManager::Render(Scene &scene, int width, int height, float alpha) 
{
    auto& sl = ServiceLocator::Instance();
    auto* graphicsContext = sl.Resolve<IGraphicsContext>();
    if (!graphicsContext) return;
    
    // 1. Shadow Pass
    RenderShadows(scene, width, height, alpha);

    // 2. Capture Pass (Optional)
    bool captured = false;
    for (IRenderSystem* sys : m_RenderCaptureSystems) {
        if (sys->IsEnabled()) {
            sys->RenderCapturePass(scene, width, height);
            captured = true;
        }
    }

    // 2.5 Rebuild Main Queue if any capture occurred (Capture passes override the global RenderQueueObj)
    if (captured) {
        auto* rs = sl.Resolve<IRenderService>();
        if (rs) rs->BuildRenderQueues(scene, alpha, width, height);
    }

    // 3. Main Render Pass
    for (IRenderSystem* sys : m_RenderMainSystems) {
        if (sys->IsEnabled()) sys->Render(scene);
    }

    // 4. Alpha Pass
    for (IRenderSystem* sys : m_RenderAlphaSystems) {
        if (sys->IsEnabled()) sys->RenderAlphaPass(scene, width, height, alpha);
    }

    // 5. Transparent Pass
    for (IRenderSystem* sys : m_RenderTransparentSystems) {
        if (sys->IsEnabled()) sys->RenderTransparentPass(scene, width, height, alpha);
    }

    // 6. PostProcess
    for (IRenderSystem* sys : m_PostProcessSystems) {
        if (sys->IsEnabled()) sys->Render(scene);
    }

    // 7. UI Pass
    IRenderStateManager* rsm = &graphicsContext->GetRenderStateManager();
    for (IRenderSystem* sys : m_RenderUISystems) {
        if (sys->IsEnabled()) sys->RenderUIPass(scene, (float)width, (float)height, *rsm);
    }
}

void SystemManager::UpdateDebug(float realDeltaTime)
{
    // DebugSystem is now updated via standard m_UpdateSystems loop (Priority 1000)
}

void SystemManager::RenderDebug(Scene& scene)
{
    for (auto& sys : m_Systems) {
        if (!sys->IsEnabled()) continue;
        
        // Explicitly drawing DebugSystem overlay if present
        if (sys->GetName() == "EditorSystem") {
            auto* context = ServiceLocator::Instance().Resolve<IGraphicsContext>();
            if (context) {
                // Width/Height from window
                auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
                if (io) {
                    int w = io->GetMonitorManager().GetWidth();
                    int h = io->GetMonitorManager().GetHeight();
                    auto& rsm = context->GetRenderStateManager();
                    
                    // We call RenderUIPass directly for DebugSystem if it's the DebugSystem
                    if (auto* renderSys = dynamic_cast<IRenderSystem*>(sys.get())) {
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
    if (rs) rs->BuildRenderQueues(scene, alpha, width, height);

    auto* shadowSys = sl.Resolve<IShadowService>();
    if (shadowSys && shadowSys->IsEnabled()) {
        shadowSys->Render(scene);
    }
}
