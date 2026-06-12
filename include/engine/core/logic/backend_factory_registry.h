#pragma once

#include <core/type/app_config.h>
#include <functional>
#include <memory>

class IAudioEngine;
class IBufferManager;
class IDrawContext;
class IGraphicsContext;
class IPhysicsWorld;
class IWindow;

class BackendFactoryRegistry
{
public:
    using GraphicsFactory = std::function<std::unique_ptr<IGraphicsContext>(const AppConfig&)>;
    using AudioFactory = std::function<std::unique_ptr<IAudioEngine>(const AppConfig&)>;
    using PhysicsFactory = std::function<std::unique_ptr<IPhysicsWorld>(const AppConfig&)>;
    using WindowFactory = std::function<std::unique_ptr<IWindow>(const AppConfig&)>;
    using PhysicsDebugRenderSetup = std::function<void(IBufferManager&, IDrawContext&)>;

    static void RegisterGraphics(GraphicsBackend backend, GraphicsFactory factory);
    static void RegisterAudio(AudioBackend backend, AudioFactory factory);
    static void RegisterPhysics(PhysicsBackend backend, PhysicsFactory factory);
    static void RegisterWindow(WindowFactory factory);
    static void RegisterPhysicsDebugRenderSetup(PhysicsDebugRenderSetup setup);

    static std::unique_ptr<IGraphicsContext> CreateGraphics(GraphicsBackend backend, const AppConfig& config);
    static std::unique_ptr<IAudioEngine> CreateAudio(AudioBackend backend, const AppConfig& config);
    static std::unique_ptr<IPhysicsWorld> CreatePhysics(PhysicsBackend backend, const AppConfig& config);
    static std::unique_ptr<IWindow> CreateWindow(const AppConfig& config);
    static void ConfigurePhysicsDebugRenderer(IBufferManager& bufferManager, IDrawContext& drawContext);
};
