#include <audio/strategy/null/null_audio_engine.h>
#include <audio/strategy/null/null_audio_capture_service.h>
#include <core/app/app_builder.h>
#include <core/logic/backend_registry.h>
#include <core/logic/logger.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <platform/strategy/opengl/glfw_window.h>
#include <render/strategy/opengl/opengl_context.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#include <platform/strategy/windows/wasapi_audio_capture_service.h>
#endif

#if AXIS_HAS_IRRKLANG_BACKEND
#include <audio/strategy/irrklang/irrklang_audio_engine.h>
#endif

#if AXIS_HAS_FMOD_BACKEND
#include <audio/strategy/fmod/fmod_audio_engine.h>
#endif

namespace
{
[[noreturn]] void ThrowUnsupportedBackend(const char* category, std::string_view requested, const char* supported)
{
    std::string message = std::string("Unsupported ") + category + " backend requested: " + std::string(requested) +
                          ". Supported in this build: " + supported + ".";
    LOGGER_ERROR("AppBuilder") << message;
    throw std::runtime_error(message);
}

template <typename T>
std::unique_ptr<T> RequireFactoryResult(std::unique_ptr<T> result, const char* category)
{
    if (result)
        return result;

    const std::string message = std::string("Custom ") + category + " factory returned null.";
    LOGGER_ERROR("AppBuilder") << message;
    throw std::runtime_error(message);
}
}  // namespace

AppBuilder& AppBuilder::WithGraphicsContextFactory(GraphicsContextFactory factory)
{
    m_GraphicsContextFactory = std::move(factory);
    return *this;
}

AppBuilder& AppBuilder::WithAudioEngineFactory(AudioEngineFactory factory)
{
    m_AudioEngineFactory = std::move(factory);
    return *this;
}

AppBuilder& AppBuilder::WithAudioCaptureFactory(AudioCaptureFactory factory)
{
    m_AudioCaptureFactory = std::move(factory);
    return *this;
}

AppBuilder& AppBuilder::WithPhysicsWorldFactory(PhysicsWorldFactory factory)
{
    m_PhysicsWorldFactory = std::move(factory);
    return *this;
}

AppBuilder& AppBuilder::WithWindowFactory(WindowFactory factory)
{
    m_WindowFactory = std::move(factory);
    return *this;
}

AppBuilder& AppBuilder::WithShaderLibrary(std::shared_ptr<IShaderLibrary> provider)
{
    m_ShaderLibrary = std::move(provider);
    return *this;
}

AppBuilder& AppBuilder::WithTextureLibrary(std::shared_ptr<ITextureLibrary> provider)
{
    m_TextureLibrary = std::move(provider);
    return *this;
}

AppBuilder& AppBuilder::WithModelLibrary(std::shared_ptr<IModelLibrary> provider)
{
    m_ModelLibrary = std::move(provider);
    return *this;
}

AppBuilder& AppBuilder::WithSoundLibrary(std::shared_ptr<ISoundLibrary> provider)
{
    m_SoundLibrary = std::move(provider);
    return *this;
}

AppBuilder& AppBuilder::WithFontLibrary(std::shared_ptr<IFontLibrary> provider)
{
    m_FontLibrary = std::move(provider);
    return *this;
}

AppBuilder& AppBuilder::WithSkyboxLibrary(std::shared_ptr<ISkyboxLibrary> provider)
{
    m_SkyboxLibrary = std::move(provider);
    return *this;
}

ApplicationProviderCapabilities AppBuilder::GetCapabilities() const
{
    ApplicationProviderCapabilities capabilities;
    capabilities.customGraphicsContext = static_cast<bool>(m_GraphicsContextFactory);
    capabilities.customAudioEngine = static_cast<bool>(m_AudioEngineFactory);
    capabilities.customAudioCapture = static_cast<bool>(m_AudioCaptureFactory);
    capabilities.customPhysicsWorld = static_cast<bool>(m_PhysicsWorldFactory);
    capabilities.customWindow = static_cast<bool>(m_WindowFactory);
    capabilities.customShaderLibrary = static_cast<bool>(m_ShaderLibrary);
    capabilities.customTextureLibrary = static_cast<bool>(m_TextureLibrary);
    capabilities.customModelLibrary = static_cast<bool>(m_ModelLibrary);
    capabilities.customSoundLibrary = static_cast<bool>(m_SoundLibrary);
    capabilities.customFontLibrary = static_cast<bool>(m_FontLibrary);
    capabilities.customSkyboxLibrary = static_cast<bool>(m_SkyboxLibrary);
    return capabilities;
}

IShaderLibrary* AppBuilder::GetShaderLibrary() const
{
    return m_ShaderLibrary.get();
}

ITextureLibrary* AppBuilder::GetTextureLibrary() const
{
    return m_TextureLibrary.get();
}

IModelLibrary* AppBuilder::GetModelLibrary() const
{
    return m_ModelLibrary.get();
}

ISoundLibrary* AppBuilder::GetSoundLibrary() const
{
    return m_SoundLibrary.get();
}

IFontLibrary* AppBuilder::GetFontLibrary() const
{
    return m_FontLibrary.get();
}

ISkyboxLibrary* AppBuilder::GetSkyboxLibrary() const
{
    return m_SkyboxLibrary.get();
}

std::unique_ptr<IGraphicsContext> AppBuilder::CreateGraphicsContext(const AppConfig& config) const
{
    if (m_GraphicsContextFactory)
        return RequireFactoryResult(m_GraphicsContextFactory(config), "graphics context");

    if (!BackendRegistry::IsSupported(config.graphics.graphicsBackend))
    {
        ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphics.graphicsBackend),
                                BackendRegistry::SupportedGraphicsBackends());
    }

    switch (config.graphics.graphicsBackend)
    {
        case GraphicsBackend::OpenGL:
            LOGGER_INFO("AppBuilder") << "Initializing Graphics Backend: OpenGL";
            return std::make_unique<OpenGLContext>();
        case GraphicsBackend::Vulkan:
        case GraphicsBackend::DirectX:
            break;
    }

    ThrowUnsupportedBackend("graphics", BackendRegistry::ToString(config.graphics.graphicsBackend),
                            BackendRegistry::SupportedGraphicsBackends());
}

std::unique_ptr<IAudioEngine> AppBuilder::CreateAudioEngine(const AppConfig& config) const
{
    if (m_AudioEngineFactory)
        return RequireFactoryResult(m_AudioEngineFactory(config), "audio engine");

    if (!BackendRegistry::IsSupported(config.audio.audioBackend))
    {
        ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audio.audioBackend),
                                BackendRegistry::SupportedAudioBackends());
    }

    switch (config.audio.audioBackend)
    {
        case AudioBackend::Null:
            LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: Null";
            return std::make_unique<NullAudioEngine>();
        case AudioBackend::IrrKlang:
#if AXIS_HAS_IRRKLANG_BACKEND
            LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: IrrKlang";
            return std::make_unique<IrrKlangAudioEngine>();
#else
            break;
#endif
        case AudioBackend::FMOD:
#if AXIS_HAS_FMOD_BACKEND
            LOGGER_INFO("AppBuilder") << "Initializing Audio Backend: FMOD";
            return std::make_unique<FMODAudioEngine>();
#else
            break;
#endif
        case AudioBackend::OpenAL:
            break;
    }

    ThrowUnsupportedBackend("audio", BackendRegistry::ToString(config.audio.audioBackend),
                            BackendRegistry::SupportedAudioBackends());
}

std::unique_ptr<IAudioCaptureService> AppBuilder::CreateAudioCaptureService() const
{
    if (m_AudioCaptureFactory)
        return RequireFactoryResult(m_AudioCaptureFactory(), "audio capture");

#if defined(_WIN32)
    return std::make_unique<WASAPIAudioCaptureService>();
#else
    return std::make_unique<NullAudioCaptureService>();
#endif
}

std::unique_ptr<IPhysicsWorld> AppBuilder::CreatePhysicsWorld(const AppConfig& config) const
{
    if (m_PhysicsWorldFactory)
        return RequireFactoryResult(m_PhysicsWorldFactory(config), "physics world");

    if (!BackendRegistry::IsSupported(config.physics.physicsBackend))
    {
        ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physics.physicsBackend),
                                BackendRegistry::SupportedPhysicsBackends());
    }

    switch (config.physics.physicsBackend)
    {
        case PhysicsBackend::Bullet:
            LOGGER_INFO("AppBuilder") << "Initializing Physics Backend: BulletPhysics";
            return std::make_unique<BulletPhysicsWorld>();
        case PhysicsBackend::PhysX:
            break;
    }

    ThrowUnsupportedBackend("physics", BackendRegistry::ToString(config.physics.physicsBackend),
                            BackendRegistry::SupportedPhysicsBackends());
}

std::unique_ptr<IWindow> AppBuilder::MakeWindow() const
{
    if (m_WindowFactory)
        return RequireFactoryResult(m_WindowFactory(), "window");
    return std::make_unique<GLFWWindow>();
}
