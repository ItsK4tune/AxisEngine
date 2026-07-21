#pragma once

#include <core/logic/event_manager.h>
#include <core/interface/i_base_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_parallel_update_system.h>
#include <ecs/interface/i_system_registry.h>
#include <ecs/interface/i_update_system.h>
#include <entt/entt.hpp>
#include <typeindex>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Application;
class IPhysicsWorld;
class ResourceManager;
struct OptimizationConfig;
struct Scene;

struct ExecutionBatch
{
    std::vector<IUpdateSystem*> systems;
};

class SystemManager : public ISystemRegistry
{
public:
    SystemManager();
    ~SystemManager();

    void CreateSystems();
    void Initialize(ResourceManager& res, int width, int height);
    void Shutdown();
    void Reset();

    void FixedUpdate(Scene& scene, float fixedDt);

    void Update(Scene& scene, float dt);

    void RenderShadows(Scene& scene, int width, int height, float alpha);
    void Render(Scene& scene, int width, int height, float alpha);

    void RenderDebug(Scene& scene);

    template <typename T>
    T* GetSystem() const
    {
        return dynamic_cast<T*>(GetSystem(std::type_index(typeid(T))));
    }

    IBaseSystem* GetSystem(const std::string& name) const override;
    IBaseSystem* GetSystem(SystemId id) const override;
    IBaseSystem* GetSystem(std::type_index concreteType) const override;
    void RegisterSystem(std::unique_ptr<IBaseSystem> system) override;

    void RebuildExecutionBatches();
    void ApplyOptimizationConfig(const OptimizationConfig& config);

private:
    struct GpuFrameTimerState;
    std::vector<std::unique_ptr<IBaseSystem>> m_Systems;
    std::vector<IBaseSystem*> m_InitializedSystems;
    std::vector<IUpdateSystem*> m_UpdateSystems;
    std::vector<IRenderSystem*> m_RenderSystems;
    std::vector<IRenderSystem*> m_RenderAlphaSystems;
    std::vector<IRenderSystem*> m_RenderTransparentSystems;
    std::vector<IRenderSystem*> m_RenderMainSystems;
    std::vector<IRenderSystem*> m_RenderUISystems;
    std::vector<IRenderSystem*> m_RenderCaptureSystems;
    std::vector<IRenderSystem*> m_PostProcessSystems;

    std::vector<ExecutionBatch> m_UpdateBatches;
    std::vector<IParallelUpdateSystem*> m_ParallelSystemsScratch;
    std::vector<FrameSnapshot> m_FrameSnapshotsScratch;
    std::vector<ECSCommandBuffer> m_CommandBuffersScratch;
    std::vector<std::future<void>> m_UpdateFuturesScratch;

    std::unordered_map<std::type_index, IBaseSystem*> m_TypeCache;
    std::unordered_map<uint64_t, IBaseSystem*> m_IdCache;
    uint32_t m_AvailableCapabilities = 0;

    struct SystemAccessSet
    {
        std::unordered_set<entt::id_type> read;
        std::unordered_set<entt::id_type> write;
    };
    std::unordered_map<IBaseSystem*, SystemAccessSet> m_SystemAccess;

    bool SystemsConflict(IUpdateSystem* a, IUpdateSystem* b) const;
    EventSubscriptionList m_EventSubscriptions;
    bool m_IsShutdown = true;
    bool m_DefaultSystemsCreated = false;
    bool m_AcceptsRegistration = true;
    std::unique_ptr<GpuFrameTimerState> m_GpuFrameTimer;
};
