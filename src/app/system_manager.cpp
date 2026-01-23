#include <app/system_manager.h>
#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <physic/physic_world.h>
#include <audio/sound_manager.h>
#include <app/application.h>
#include <input/mouse_manager.h>
#include <utils/logger.h>
#include <iostream>

#include <glad/glad.h>

SystemManager::SystemManager()
{
}

SystemManager::~SystemManager()
{
}

void SystemManager::InitializeSystems(ResourceManager& res, int width, int height, Application* app)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";
    postProcess.Init(width, height, res);
    renderSystem.InitShadows(res);

#ifdef ENABLE_DEBUG_SYSTEM
    debugSystem = std::make_unique<DebugSystem>();
    debugSystem->Init(app);
#endif
}

void SystemManager::ShutdownSystems()
{
    LOGGER_INFO("SystemManager") << "Shutting down systems...";
    renderSystem.Shutdown();
    postProcess.Shutdown();
}

void SystemManager::FixedUpdateSystems(Scene& scene, PhysicsWorld& phys, float fixedDt)
{
    physicsSystem.Update(scene, phys, fixedDt);
}

void SystemManager::UpdateSystems(Scene& scene, float deltaTime, float realDeltaTime,
                                  Application* app, ResourceManager& res,
                                  SoundManager& sound, MouseManager& mouse)
{
    scriptSystem.Update(scene, deltaTime, realDeltaTime, app);
    animationSystem.Update(scene, deltaTime);
    videoSystem.Update(scene, res, deltaTime);
    uiInteractSystem.Update(scene, deltaTime, mouse);
    audioSystem.Update(scene, sound);
    particleSystem.Update(scene, deltaTime);
}

void SystemManager::RenderShadows(Scene& scene)
{
    renderSystem.RenderShadows(scene);
}

void SystemManager::RenderSystems(Scene& scene, ResourceManager& res, int width, int height)
{
    glViewport(0, 0, width, height);
    postProcess.BeginCapture();

    skyboxRenderSystem.Render(scene);
    renderSystem.Render(scene, width, height);
    particleSystem.Render(scene, res);

    uiRenderSystem.Render(scene, (float)width, (float)height);

    postProcess.ApplyAntiAliasing(renderSystem.GetAntiAliasingMode(), 
                                  renderSystem.GetPrevViewProj(), 
                                  renderSystem.GetCurrViewProj(), 
                                  renderSystem.GetJitterOffset());

    postProcess.EndCapture();
}

#ifdef ENABLE_DEBUG_SYSTEM
void SystemManager::UpdateDebugSystem(float realDeltaTime)
{
    if (debugSystem)
        debugSystem->OnUpdate(realDeltaTime);
}

void SystemManager::RenderDebugSystem(Scene& scene)
{
    if (debugSystem)
        debugSystem->Render(scene);
}
#endif
