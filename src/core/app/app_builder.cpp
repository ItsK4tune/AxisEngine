#include <core/app/app_builder.h>
#include <systems/window/backends/glfw_window.h>
#include <systems/audio/backends/irrklang_audio_engine.h>
#include <rendering/backends/opengl_context.h>
#include <systems/physics/backends/bullet_physics_world.h>
#include <core/utils/logger.h>

std::unique_ptr<IGraphicsContext> AppBuilder::CreateGraphicsContext(const AppConfig &config)
{
    if (config.graphicsBackend == "OPENGL")
    {
        LOGGER_INFO("AppBuilder") << "Initializing Graphics Backend: OpenGL";
        return std::make_unique<OpenGLContext>();
    }

    LOGGER_ERROR("AppBuilder") << "Unsupported Graphics Backend: " << config.graphicsBackend << ". Defaulting to OpenGL.";
    return std::make_unique<OpenGLContext>();
}

std::unique_ptr<IAudioEngine> AppBuilder::CreateAudioEngine(const AppConfig &config)
{
    if (config.audioBackend == "IRRKLANG")
    {
        LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: IrrKlang";
        return std::make_unique<IrrKlangAudioEngine>();
    }

    LOGGER_ERROR("AppBuilder") << "Unsupported Audio Backend: " << config.audioBackend << ". Defaulting to IrrKlang.";
    return std::make_unique<IrrKlangAudioEngine>();
}

std::unique_ptr<IPhysicsWorld> AppBuilder::CreatePhysicsWorld(const AppConfig &config)
{
    if (config.physicsBackend == "BULLET")
    {
        LOGGER_INFO("AppBuilder") << "Initializing Physics Backend: BulletPhysics";
        return std::make_unique<BulletPhysicsWorld>();
    }

    LOGGER_ERROR("AppBuilder") << "Unsupported Physics Backend: " << config.physicsBackend << ". Defaulting to BulletPhysics.";
    return std::make_unique<BulletPhysicsWorld>();
}

std::unique_ptr<IWindow> AppBuilder::MakeWindow()
{
    return std::make_unique<GLFWWindow>();
}
