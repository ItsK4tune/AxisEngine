#pragma once

#include <ecs/interface/i_base_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <bitset>
#include <typeindex>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Application;
class IPhysicsWorld;
class ResourceManager;
struct Scene;

struct ExecutionBatch
{
    std::vector<IUpdateSystem*> systems;
};

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void CreateSystems();
    void Initialize(ResourceManager& res, int width, int height);
    void Shutdown();

    void FixedUpdate(Scene& scene, float fixedDt);

    void Update(Scene& scene, float dt);

    void RenderShadows(Scene& scene, int width, int height, float alpha);
    void Render(Scene& scene, int width, int height, float alpha);

    void UpdateDebug(float realDeltaTime);
    void RenderDebug(Scene& scene);

    template <typename T>
    T* GetSystem() const
    {
        auto it = m_TypeCache.find(std::type_index(typeid(T)));
        if (it != m_TypeCache.end())
            return dynamic_cast<T*>(it->second);
        return nullptr;
    }

    IBaseSystem* GetSystem(const std::string& name) const;
    void RegisterSystem(std::unique_ptr<IBaseSystem> system);

    void RebuildExecutionBatches();

private:
    std::vector<std::unique_ptr<IBaseSystem>> m_Systems;
    std::vector<IUpdateSystem*> m_UpdateSystems;
    std::vector<IRenderSystem*> m_RenderSystems;
    std::vector<IRenderSystem*> m_RenderAlphaSystems;
    std::vector<IRenderSystem*> m_RenderTransparentSystems;
    std::vector<IRenderSystem*> m_RenderMainSystems;
    std::vector<IRenderSystem*> m_RenderUISystems;
    std::vector<IRenderSystem*> m_RenderCaptureSystems;
    std::vector<IRenderSystem*> m_PostProcessSystems;

    std::vector<ExecutionBatch> m_UpdateBatches;

    std::unordered_map<std::type_index, IBaseSystem*> m_TypeCache;
    uint32_t m_AvailableCapabilities = 0;

    // Bitset optimization for SystemsConflict
    struct SystemBitset
    {
        std::bitset<128> read;
        std::bitset<128> write;
    };
    std::unordered_map<IBaseSystem*, SystemBitset> m_SystemBitsets;
    static uint32_t GetComponentBitIndex(entt::id_type id);

    bool SystemsConflict(IUpdateSystem* a, IUpdateSystem* b) const;
};
