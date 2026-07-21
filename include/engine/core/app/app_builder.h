#pragma once

#include <core/type/app_config.h>
#include <resource/interface/i_resource_libraries.h>
#include <functional>
#include <memory>

class IAudioCaptureService;
class IAudioEngine;
class Application;
class IGraphicsContext;
class IPhysicsWorld;
class IWindow;

struct ApplicationProviderCapabilities
{
    bool customGraphicsContext = false;
    bool customAudioEngine = false;
    bool customAudioCapture = false;
    bool customPhysicsWorld = false;
    bool customWindow = false;
    bool customShaderLibrary = false;
    bool customTextureLibrary = false;
    bool customModelLibrary = false;
    bool customSoundLibrary = false;
    bool customFontLibrary = false;
    bool customSkyboxLibrary = false;
};

class AppBuilder
{
public:
    using GraphicsContextFactory = std::function<std::unique_ptr<IGraphicsContext>(const AppConfig&)>;
    using AudioEngineFactory = std::function<std::unique_ptr<IAudioEngine>(const AppConfig&)>;
    using AudioCaptureFactory = std::function<std::unique_ptr<IAudioCaptureService>()>;
    using PhysicsWorldFactory = std::function<std::unique_ptr<IPhysicsWorld>(const AppConfig&)>;
    using WindowFactory = std::function<std::unique_ptr<IWindow>()>;

    // Instance-bound providers. Pass the configured builder to Application so
    // multiple engine instances or tests do not mutate process-global state.
    AppBuilder& WithGraphicsContextFactory(GraphicsContextFactory factory);
    AppBuilder& WithAudioEngineFactory(AudioEngineFactory factory);
    AppBuilder& WithAudioCaptureFactory(AudioCaptureFactory factory);
    AppBuilder& WithPhysicsWorldFactory(PhysicsWorldFactory factory);
    AppBuilder& WithWindowFactory(WindowFactory factory);
    AppBuilder& WithShaderLibrary(std::shared_ptr<IShaderLibrary> provider);
    AppBuilder& WithTextureLibrary(std::shared_ptr<ITextureLibrary> provider);
    AppBuilder& WithModelLibrary(std::shared_ptr<IModelLibrary> provider);
    AppBuilder& WithSoundLibrary(std::shared_ptr<ISoundLibrary> provider);
    AppBuilder& WithFontLibrary(std::shared_ptr<IFontLibrary> provider);
    AppBuilder& WithSkyboxLibrary(std::shared_ptr<ISkyboxLibrary> provider);
    ApplicationProviderCapabilities GetCapabilities() const;

private:
    friend class Application;
    std::unique_ptr<IGraphicsContext> CreateGraphicsContext(const AppConfig& config) const;
    std::unique_ptr<IAudioEngine> CreateAudioEngine(const AppConfig& config) const;
    std::unique_ptr<IAudioCaptureService> CreateAudioCaptureService() const;
    std::unique_ptr<IPhysicsWorld> CreatePhysicsWorld(const AppConfig& config) const;
    std::unique_ptr<IWindow> MakeWindow() const;
    IShaderLibrary* GetShaderLibrary() const;
    ITextureLibrary* GetTextureLibrary() const;
    IModelLibrary* GetModelLibrary() const;
    ISoundLibrary* GetSoundLibrary() const;
    IFontLibrary* GetFontLibrary() const;
    ISkyboxLibrary* GetSkyboxLibrary() const;

    GraphicsContextFactory m_GraphicsContextFactory;
    AudioEngineFactory m_AudioEngineFactory;
    AudioCaptureFactory m_AudioCaptureFactory;
    PhysicsWorldFactory m_PhysicsWorldFactory;
    WindowFactory m_WindowFactory;
    std::shared_ptr<IShaderLibrary> m_ShaderLibrary;
    std::shared_ptr<ITextureLibrary> m_TextureLibrary;
    std::shared_ptr<IModelLibrary> m_ModelLibrary;
    std::shared_ptr<ISoundLibrary> m_SoundLibrary;
    std::shared_ptr<IFontLibrary> m_FontLibrary;
    std::shared_ptr<ISkyboxLibrary> m_SkyboxLibrary;
};
