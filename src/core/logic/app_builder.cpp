#include <core/logic/app_framework.h>
#include <platform/strategy/opengl/glfw_window.h>
#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <render/strategy/opengl/opengl_context.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <core/logic/logger.h>

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
