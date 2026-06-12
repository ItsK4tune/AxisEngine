#include <core/app/application.h>
#include <core/logic/backend_factory_registry.h>
#include <core/logic/backend_registry.h>
#include <core/logic/compiled_backend_registration.h>
#include <core/logic/logger.h>
#include <render/interface/i_graphics_context.h>
#include <audio/interface/i_audio_engine.h>
#include <physics/interface/i_physics_world.h>
#include <stdexcept>
#include <string>

namespace
{
[[noreturn]] void ThrowUnsupportedBackend(const char* category, std::string_view requested, const char* supported)
{
    std::string message = std::string("Unsupported ") + category + " backend requested: " + std::string(requested) +
                          ". Supported in this build: " + supported + ".";
    LOGGER_ERROR("AppBuilder") << message;
    throw std::runtime_error(message);
}
}  // namespace

std::unique_ptr<IGraphicsContext> AppBuilder::CreateGraphicsContext(const AppConfig& config)
{
    RegisterCompiledBackendFactories();

    if (!BackendRegistry::IsSupported(config.graphicsBackend))
    {
        ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphicsBackend),
                                BackendRegistry::SupportedGraphicsBackends());
    }

    if (auto context = BackendFactoryRegistry::CreateGraphics(config.graphicsBackend, config))
        return context;

    ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphicsBackend),
                            BackendRegistry::SupportedGraphicsBackends());
}

std::unique_ptr<IAudioEngine> AppBuilder::CreateAudioEngine(const AppConfig& config)
{
    RegisterCompiledBackendFactories();

    if (!BackendRegistry::IsSupported(config.audioBackend))
    {
        ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audioBackend),
                                BackendRegistry::SupportedAudioBackends());
    }

    if (auto engine = BackendFactoryRegistry::CreateAudio(config.audioBackend, config))
        return engine;

    ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audioBackend),
                            BackendRegistry::SupportedAudioBackends());
}

std::unique_ptr<IPhysicsWorld> AppBuilder::CreatePhysicsWorld(const AppConfig& config)
{
    RegisterCompiledBackendFactories();

    if (!BackendRegistry::IsSupported(config.physicsBackend))
    {
        ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physicsBackend),
                                BackendRegistry::SupportedPhysicsBackends());
    }

    if (auto world = BackendFactoryRegistry::CreatePhysics(config.physicsBackend, config))
        return world;

    ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physicsBackend),
                            BackendRegistry::SupportedPhysicsBackends());
}

std::unique_ptr<IWindow> AppBuilder::MakeWindow(const AppConfig& config)
{
    RegisterCompiledBackendFactories();
    auto window = BackendFactoryRegistry::CreateWindow(config);
    if (!window)
        throw std::runtime_error("No platform window backend registered in this build.");
    return window;
}
