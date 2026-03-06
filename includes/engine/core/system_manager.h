#pragma once

#include <app/config_loader.h>
#include <core/engine_context.h>
#include <ecs/i_system.h>
#include <graphics/core/post_process_pipeline.h>
#include <debug/i_debug_system.h>
#include <memory>
#include <vector>
#include <string>

class Scene;
class ResourceManager;
class IPhysicsWorld;
class SoundPlayer;
class Application;
class MouseManager;

class SystemManager
{
public:
    SystemManager();
    ~SystemManager();

    void InitializeSystems(ResourceManager &res, int width, int height, EngineContext ctx);
    void ApplyConfig(const AppConfig &config);
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

    ISystem* GetSystem(const std::string& name) const;
    void RegisterSystem(std::unique_ptr<ISystem> system);

    PostProcessPipeline &GetPostProcess() { return postProcess; }

private:
    std::vector<std::unique_ptr<ISystem>> m_Systems;
    PostProcessPipeline postProcess;

    EngineContext m_Ctx;

    std::unique_ptr<IDebugSystem> m_DebugSystem;
};

