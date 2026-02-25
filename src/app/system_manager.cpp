#include <utils/logger.h>
#include <app/system_manager.h>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_render_state_manager.h>
#include <interface/graphic/i_buffer_manager.h>
#include <interface/graphic/i_texture_manager.h>
#include <interface/graphic/i_shader_manager.h>
#include <interface/graphic/i_render_target_manager.h>
#include <interface/graphic/i_draw_context.h>
#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <interface/physics/i_physics_world.h>
#include <audio/sound_player.h>
#include <app/application.h>
#include <app/io_handler.h>
#include <input/mouse_manager.h>
#include <utils/logger.h>
#include <iostream>

SystemManager::SystemManager()
{
}

SystemManager::~SystemManager()
{
}

void SystemManager::InitializeSystems(ResourceManager& res, int width, int height, Application* app)
{
    LOGGER_INFO("SystemManager") << "Initializing systems...";

    if (app) {
        auto& ioHandler = app->GetIOHandler();
        auto& context = ioHandler.GetGraphicsContext();
        postProcess.Init(context, width, height, res);
        renderSystem.Init(context, res);
        skyboxRenderSystem.Init(context);
        particleSystem.Init(context);
    } else {
        LOGGER_ERROR("SystemManager") << "Application is null, cannot init systems";
    }

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

void SystemManager::ApplyConfig(const AppConfig &config)
{
    renderSystem.SetShadowMode(config.shadowMode);
    renderSystem.SetShadowProjectionSize(config.shadowProjectionSize);
    renderSystem.SetInstanceBatching(config.instanceBatchingEnabled);
    renderSystem.SetFrustumCulling(config.frustumCullingEnabled);
    renderSystem.SetOcclusionCulling(config.occlusionCullingEnabled);
    renderSystem.SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
    renderSystem.SetShadowDistanceCulling(config.shadowDistanceCulling);
    renderSystem.SetDistanceCulling(config.distanceCulling);
    renderSystem.SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
}

void SystemManager::FixedUpdateSystems(Scene& scene, IPhysicsWorld& phys, float fixedDt, uint32_t mask)
{
    if (mask & (uint32_t)SystemGroup::Physics)
        physicsSystem.Update(scene, phys, fixedDt);
}

void SystemManager::UpdateLogic(Scene& scene, float deltaTime, float realDeltaTime, Application* app, MouseManager& mouse, uint32_t mask)
{
    if (mask & (uint32_t)SystemGroup::Script)
        scriptSystem.Update(scene, deltaTime, realDeltaTime, app);
}

void SystemManager::UpdateVisuals(Scene& scene, float deltaTime, ResourceManager& res, SoundPlayer& sound, uint32_t mask)
{
    if (mask & (uint32_t)SystemGroup::Animation)
        animationSystem.Update(scene, deltaTime);
    if (mask & (uint32_t)SystemGroup::Video)
        videoSystem.Update(scene, res, deltaTime);
    if (mask & (uint32_t)SystemGroup::Audio)
        audioSystem.Update(scene, sound);
    if (mask & (uint32_t)SystemGroup::Particle)
        particleSystem.Update(scene, deltaTime);
}


void SystemManager::RenderShadows(Scene& scene, uint32_t mask)
{
    if (mask & (uint32_t)SystemGroup::Render)
        renderSystem.RenderShadows(scene);
}

void SystemManager::RenderSystems(Scene& scene, ResourceManager& res, int width, int height, uint32_t mask)
{
    if (renderSystem.GetContext())
        renderSystem.GetContext()->GetRenderStateManager().Viewport(0, 0, width, height);
    
    postProcess.BeginCapture();

    if (mask & (uint32_t)SystemGroup::Skybox)
        skyboxRenderSystem.Render(scene);
    
    if (mask & (uint32_t)SystemGroup::Render)
        renderSystem.Render(scene, width, height);
    
    if (mask & (uint32_t)SystemGroup::Particle)
        particleSystem.Render(scene, res);

    if (mask & (uint32_t)SystemGroup::UI)
        uiRenderSystem.Render(scene, (float)width, (float)height, renderSystem.GetContext()->GetRenderStateManager());

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
