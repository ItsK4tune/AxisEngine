#pragma once

#include <core/logic/config_loader.h>
#include <core/logic/debug_system.h>
#include <ecs/interface/i_base_system.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>
#include <memory>
#include <render/logic/post_process_pipeline.h>
#include <string>
#include <vector>
#include <set>

class Application;
class IPhysicsWorld;
class ResourceManager;
class Scene;
class IDebugSystem;

struct ExecutionBatch {
    std::vector<IUpdateSystem*> systems;
};

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void CreateSystems();
    void InitializeSystems(ResourceManager &res, int width, int height);
    void Shutdown();

    void RunFixedUpdate(Scene &scene, float fixedDt);

    void RunUpdate(Scene &scene, float dt);

    void RenderShadows(Scene &scene, float alpha);
    void RunRender(Scene &scene, int width, int height, float alpha);

    void UpdateDebugSystem(float realDeltaTime);
    void RenderDebugSystem(Scene& scene);
    IDebugSystem* GetDebugSystem() { return m_DebugSystem.get(); }

    template<typename T>
    T* GetSystem() const
    {
        for (const auto& sys : m_Systems)
        {
            if (T* casted = dynamic_cast<T*>(sys.get()))
                return casted;
        }
        return nullptr;
    }

    IBaseSystem* GetSystem(const std::string& name) const;
    void RegisterSystem(std::unique_ptr<IBaseSystem> system);

    PostProcessPipeline &GetPostProcess() { return postProcess; }

    void RebuildExecutionBatches();

private:
    std::vector<std::unique_ptr<IBaseSystem>> m_Systems;
    std::vector<IUpdateSystem*> m_UpdateSystems;
    std::vector<IRenderSystem*> m_RenderSystems;
    
    std::vector<ExecutionBatch> m_UpdateBatches;
    
    PostProcessPipeline postProcess;

    std::unique_ptr<IDebugSystem> m_DebugSystem;

    bool SystemsConflict(IUpdateSystem* a, IUpdateSystem* b) const;
};