#include <core/logic/application.h>
#include <platform/strategy/opengl/glfw_window.h>
#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#include <render/strategy/opengl/opengl_context.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <core/logic/logger.h>

std::unique_ptr<IGraphicsContext> AppBuilder::CreateGraphicsContext(const AppConfig &config)
{
    switch (config.graphicsBackend)
    {
    case GraphicsBackend::OpenGL:
        LOGGER_INFO("AppBuilder") << "Initializing Graphics Backend: OpenGL";
        return std::make_unique<OpenGLContext>();
    case GraphicsBackend::Vulkan:
        LOGGER_ERROR("AppBuilder") << "Vulkan Backend not yet implemented. Defaulting to OpenGL.";
        return std::make_unique<OpenGLContext>();
    case GraphicsBackend::DirectX:
        LOGGER_ERROR("AppBuilder") << "DirectX Backend not yet implemented. Defaulting to OpenGL.";
        return std::make_unique<OpenGLContext>();
    }

    return std::make_unique<OpenGLContext>();
}

std::unique_ptr<IAudioEngine> AppBuilder::CreateAudioEngine(const AppConfig &config)
{
    switch (config.audioBackend)
    {
    case AudioBackend::IrrKlang:
        LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: IrrKlang";
        return std::make_unique<IrrKlangAudioEngine>();
    case AudioBackend::FMOD:
        LOGGER_ERROR("AppBuilder") << "FMOD Backend not yet implemented. Defaulting to IrrKlang.";
        return std::make_unique<IrrKlangAudioEngine>();
    case AudioBackend::OpenAL:
        LOGGER_ERROR("AppBuilder") << "OpenAL Backend not yet implemented. Defaulting to IrrKlang.";
        return std::make_unique<IrrKlangAudioEngine>();
    }

    return std::make_unique<IrrKlangAudioEngine>();
}

std::unique_ptr<IPhysicsWorld> AppBuilder::CreatePhysicsWorld(const AppConfig &config)
{
    switch (config.physicsBackend)
    {
    case PhysicsBackend::Bullet:
        LOGGER_INFO("AppBuilder") << "Initializing Physics Backend: BulletPhysics";
        return std::make_unique<BulletPhysicsWorld>();
    case PhysicsBackend::PhysX:
        LOGGER_ERROR("AppBuilder") << "PhysX Backend not yet implemented. Defaulting to Bullet.";
        return std::make_unique<BulletPhysicsWorld>();
    }

    return std::make_unique<BulletPhysicsWorld>();
}

std::unique_ptr<IWindow> AppBuilder::MakeWindow()
{
    return std::make_unique<GLFWWindow>();
}
