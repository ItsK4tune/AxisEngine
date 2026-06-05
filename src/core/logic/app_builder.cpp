#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <core/app/application.h>
#include <core/logic/backend_registry.h>
#include <core/logic/logger.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <platform/strategy/opengl/glfw_window.h>
#include <render/strategy/opengl/opengl_context.h>
#include <stdexcept>
#include <string>

namespace
{
[[noreturn]] void ThrowUnsupportedBackend(const char* category, std::string_view requested, const char* supported)
{
    std::string message = std::string("Unsupported ") + category + " backend requested: " +
                          std::string(requested) + ". Supported in this build: " + supported + ".";
    LOGGER_ERROR("AppBuilder") << message;
    throw std::runtime_error(message);
}
}  // namespace

std::unique_ptr<IGraphicsContext> AppBuilder::CreateGraphicsContext(const AppConfig& config)
{
    if (!BackendRegistry::IsSupported(config.graphicsBackend))
    {
        ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphicsBackend),
                                BackendRegistry::SupportedGraphicsBackends());
    }

    switch (config.graphicsBackend)
    {
        case GraphicsBackend::OpenGL:
            LOGGER_INFO("AppBuilder") << "Initializing Graphics Backend: OpenGL";
            return std::make_unique<OpenGLContext>();
        case GraphicsBackend::Vulkan:
        case GraphicsBackend::DirectX:
            break;
    }

    ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphicsBackend),
                            BackendRegistry::SupportedGraphicsBackends());
}

std::unique_ptr<IAudioEngine> AppBuilder::CreateAudioEngine(const AppConfig& config)
{
    if (!BackendRegistry::IsSupported(config.audioBackend))
    {
        ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audioBackend),
                                BackendRegistry::SupportedAudioBackends());
    }

    switch (config.audioBackend)
    {
        case AudioBackend::IrrKlang:
            LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: IrrKlang";
            return std::make_unique<IrrKlangAudioEngine>();
        case AudioBackend::FMOD:
        case AudioBackend::OpenAL:
            break;
    }

    ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audioBackend),
                            BackendRegistry::SupportedAudioBackends());
}

std::unique_ptr<IPhysicsWorld> AppBuilder::CreatePhysicsWorld(const AppConfig& config)
{
    if (!BackendRegistry::IsSupported(config.physicsBackend))
    {
        ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physicsBackend),
                                BackendRegistry::SupportedPhysicsBackends());
    }

    switch (config.physicsBackend)
    {
        case PhysicsBackend::Bullet:
            LOGGER_INFO("AppBuilder") << "Initializing Physics Backend: BulletPhysics";
            return std::make_unique<BulletPhysicsWorld>();
        case PhysicsBackend::PhysX:
            break;
    }

    ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physicsBackend),
                            BackendRegistry::SupportedPhysicsBackends());
}

std::unique_ptr<IWindow> AppBuilder::MakeWindow()
{
    return std::make_unique<GLFWWindow>();
}
