#include <core/logic/backend_factory_registry.h>

#include <audio/interface/i_audio_engine.h>
#include <physics/interface/i_physics_world.h>
#include <platform/interface/i_window.h>
#include <render/interface/i_graphics_context.h>
#include <unordered_map>
#include <utility>

namespace
{
using GraphicsFactoryMap = std::unordered_map<GraphicsBackend, BackendFactoryRegistry::GraphicsFactory>;
using AudioFactoryMap = std::unordered_map<AudioBackend, BackendFactoryRegistry::AudioFactory>;
using PhysicsFactoryMap = std::unordered_map<PhysicsBackend, BackendFactoryRegistry::PhysicsFactory>;

GraphicsFactoryMap& GraphicsFactories()
{
    static GraphicsFactoryMap factories;
    return factories;
}

AudioFactoryMap& AudioFactories()
{
    static AudioFactoryMap factories;
    return factories;
}

PhysicsFactoryMap& PhysicsFactories()
{
    static PhysicsFactoryMap factories;
    return factories;
}

BackendFactoryRegistry::WindowFactory& WindowFactoryRef()
{
    static BackendFactoryRegistry::WindowFactory factory;
    return factory;
}

BackendFactoryRegistry::PhysicsDebugRenderSetup& PhysicsDebugRenderSetupRef()
{
    static BackendFactoryRegistry::PhysicsDebugRenderSetup setup;
    return setup;
}
}  // namespace

void BackendFactoryRegistry::RegisterGraphics(GraphicsBackend backend, GraphicsFactory factory)
{
    GraphicsFactories()[backend] = std::move(factory);
}

void BackendFactoryRegistry::RegisterAudio(AudioBackend backend, AudioFactory factory)
{
    AudioFactories()[backend] = std::move(factory);
}

void BackendFactoryRegistry::RegisterPhysics(PhysicsBackend backend, PhysicsFactory factory)
{
    PhysicsFactories()[backend] = std::move(factory);
}

void BackendFactoryRegistry::RegisterWindow(WindowFactory factory)
{
    WindowFactoryRef() = std::move(factory);
}

void BackendFactoryRegistry::RegisterPhysicsDebugRenderSetup(PhysicsDebugRenderSetup setup)
{
    PhysicsDebugRenderSetupRef() = std::move(setup);
}

std::unique_ptr<IGraphicsContext> BackendFactoryRegistry::CreateGraphics(GraphicsBackend backend,
                                                                         const AppConfig& config)
{
    auto it = GraphicsFactories().find(backend);
    return it != GraphicsFactories().end() ? it->second(config) : nullptr;
}

std::unique_ptr<IAudioEngine> BackendFactoryRegistry::CreateAudio(AudioBackend backend, const AppConfig& config)
{
    auto it = AudioFactories().find(backend);
    return it != AudioFactories().end() ? it->second(config) : nullptr;
}

std::unique_ptr<IPhysicsWorld> BackendFactoryRegistry::CreatePhysics(PhysicsBackend backend, const AppConfig& config)
{
    auto it = PhysicsFactories().find(backend);
    return it != PhysicsFactories().end() ? it->second(config) : nullptr;
}

std::unique_ptr<IWindow> BackendFactoryRegistry::CreateWindow(const AppConfig& config)
{
    auto& factory = WindowFactoryRef();
    return factory ? factory(config) : nullptr;
}

void BackendFactoryRegistry::ConfigurePhysicsDebugRenderer(IBufferManager& bufferManager, IDrawContext& drawContext)
{
    auto& setup = PhysicsDebugRenderSetupRef();
    if (setup)
        setup(bufferManager, drawContext);
}
