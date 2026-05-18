#pragma once

#include <core/interface/i_loader_strategy.h>
#include <render/interface/i_shader_manager.h>
#include <resource/interface/i_resource_libraries.h>
#include <resource/logic/animation_manager.h>
#include <resource/logic/audio_asset_manager.h>
#include <resource/logic/font_manager.h>
#include <resource/logic/fragment_asset_manager.h>
#include <resource/logic/model_instance_manager.h>
#include <resource/logic/model_manager.h>
#include <resource/logic/resource_watcher.h>
#include <resource/logic/shader_manager.h>
#include <resource/logic/skybox_manager.h>
#include <resource/logic/texture_manager.h>
#include <resource/logic/ui_model_manager.h>
#include <resource/logic/video_manager.h>
#include <resource/unit/ui_model.h>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class ResourceManager : public IShaderLibrary,
                        public ITextureLibrary,
                        public IModelLibrary,
                        public ISoundLibrary,
                        public IFontLibrary,
                        public ISkyboxLibrary
{
public:
    ResourceManager();
    ~ResourceManager();

    void Initialize(IShaderManager* shaderManager, ITextureManager* textureManager, IAudioEngine& audioEngine);
    void InitializeHeadless();  // No GPU managers, no audio engine
    void InitializePostLoad();

    bool IsHeadless() const
    {
        return m_HeadlessMode;
    }
    void Shutdown();
    void Update(float dt);
    void SetTextureAsyncEnabled(bool enabled);
    void SetTextureMaxAnisotropy(float max);

    void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath,
                    const std::string& gsPath = "");
    void LoadTexture(const std::string& name, const std::string& path, bool async = true,
                     bool keepCpuData = false) override;
    void UnloadTexture(const std::string& name);

    void LoadModel(const std::string& name, const std::string& path, bool isStatic = false);
    void LoadModelAsync(const std::string& name, const std::string& path, bool isStatic = false);
    void UnloadModel(const std::string& name);

    void LoadAnimation(const std::string& name, const std::string& path, const std::string& modelName);
    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine);
    void LoadSkybox(const std::string& name, const std::vector<std::string>& faces);
    void LoadFragment(const std::string& name, const std::string& path);
    void CreateUIModel(const std::string& name, UIType type);

    void UnloadShader(const std::string& name);
    void UnloadFont(const std::string& name);
    void UnloadSound(const std::string& name);
    void UnloadSkybox(const std::string& name);
    void UnloadAnimation(const std::string& name);
    void UnloadFragment(const std::string& name);

    std::shared_ptr<Shader> GetShader(const std::string& name);
    std::shared_ptr<Texture> GetTexture(const std::string& name);
    std::shared_ptr<Model> GetModel(const std::string& name);
    std::shared_ptr<Animation> GetAnimation(const std::string& name);
    std::shared_ptr<Font> GetFont(const std::string& name);
    std::shared_ptr<IAudioSource> GetSound(const std::string& name);
    std::shared_ptr<Skybox> GetSkybox(const std::string& name);
    std::shared_ptr<FragmentAsset> GetFragment(const std::string& name);
    std::shared_ptr<UIModel> GetUIModel(const std::string& name);
    bool HasUIModel(const std::string& name);
    std::shared_ptr<VideoDecoder> GetVideo(const std::string& name);

    std::vector<std::string> GetLoadedTextures() const;
    std::vector<std::string> GetLoadedModels() const;
    std::vector<std::string> GetLoadedShaders() const;
    std::vector<std::string> GetLoadedSounds() const;
    std::vector<std::string> GetLoadedSkyboxes() const;
    std::vector<std::string> GetLoadedAnimations() const;
    std::vector<std::string> GetLoadedVideos() const;
    std::vector<std::string> GetLoadedFonts() const;
    std::vector<std::string> GetLoadedFragments() const;

    std::shared_ptr<Texture> GetTextureAuto(const std::string& nameOrPath);
    std::shared_ptr<Model> GetModelAuto(const std::string& nameOrPath, bool isStatic = false);
    std::shared_ptr<Font> GetFontAuto(const std::string& nameOrPath, unsigned int fontSize = 16);
    std::shared_ptr<IAudioSource> GetSoundAuto(const std::string& nameOrPath, IAudioEngine* engine = nullptr);

    void ClearResource();

    struct ResourceDefinition
    {
        std::string type;  // "Shader", "Texture", "Model", "Font", "Skybox", "Sound"
        std::string name;
        std::unordered_map<std::string, std::string> properties;
    };
    const std::vector<ResourceDefinition>& GetResourceDefinitions() const
    {
        return m_ResourceDefinitions;
    }
    void AddResourceDefinition(const std::string& type, const std::string& name,
                               const std::unordered_map<std::string, std::string>& props);

    void RegisterLoader(std::unique_ptr<ILoaderStrategy> strategy);
    bool LoadUnified(const std::string& type, const std::string& path);
    std::vector<std::string> GetRegisteredLoaderTypes() const;

private:
    void ReloadShader(const std::string& name);
    void ReloadTexture(const std::string& name);

    std::unique_ptr<ShaderManager> m_ShaderManager;
    std::unique_ptr<TextureManager> m_TextureManager;
    std::unique_ptr<ModelManager> m_ModelManager;
    std::unique_ptr<AudioAssetManager> m_AudioManager;
    std::unique_ptr<FontManager> m_FontManager;
    std::unique_ptr<SkyboxManager> m_SkyboxManager;
    std::unique_ptr<AnimationManager> m_AnimationManager;
    std::unique_ptr<VideoManager> m_VideoManager;
    std::unique_ptr<FragmentAssetManager> m_FragmentManager;
    std::unique_ptr<UIModelManager> m_UIModelManager;

    ModelInstanceManager m_ModelInstanceManager;
    ResourceWatcher m_ResourceWatcher;

    std::mutex m_ResourceMutex;

    std::unordered_map<std::string, std::unique_ptr<ILoaderStrategy>> m_Strategies;

    std::vector<ResourceDefinition> m_ResourceDefinitions;

    bool m_HeadlessMode = false;
    int m_ReloadListenerId = -1;
};
