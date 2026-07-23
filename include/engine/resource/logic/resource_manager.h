#pragma once

#include <core/interface/i_loader_strategy.h>
#include <render/interface/i_shader_manager.h>
#include <resource/interface/i_resource_libraries.h>
#include <resource/logic/animation_manager.h>
#include <resource/logic/audio_asset_manager.h>
#include <resource/logic/font_manager.h>
#include <resource/logic/fragment_asset_manager.h>
#include <resource/logic/model_manager.h>
#include <resource/logic/resource_watcher.h>
#include <resource/logic/shader_manager.h>
#include <resource/logic/skybox_manager.h>
#include <resource/logic/texture_manager.h>
#include <resource/logic/ui_model_manager.h>
#include <resource/logic/video_manager.h>
#include <resource/unit/ui_model.h>
#include <resource/unit/compute_shader.h>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
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

    void Initialize(IShaderManager* shaderManager, ITextureManager* textureManager, IAudioEngine* audioEngine);
    void Initialize(IShaderManager* shaderManager, ITextureManager* textureManager, IAudioEngine& audioEngine)
    {
        Initialize(shaderManager, textureManager, &audioEngine);
    }
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
    void SetStrictAssetLoading(bool strict);
    void SetAsyncUploadBudget(bool enabled, size_t maxModelUploadsPerFrame, size_t maxTextureUploadsPerFrame);
    void ApplyOptimizationConfig(const struct OptimizationConfig& config);
    bool IsStrictAssetLoading() const
    {
        return m_StrictAssetLoading;
    }

    void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath,
                    const std::string& gsPath = "") override;
    bool LoadComputeShader(const std::string& name, const std::string& path);
    void LoadTexture(const std::string& name, const std::string& path, bool async = true,
                     bool keepCpuData = false) override;
    void CreateTextureFromData(const std::string& name, const unsigned char* pixels, int width, int height,
                               int nrComponents, bool keepCpuData = false);
    void UnloadTexture(const std::string& name) override;

    void LoadModel(const std::string& name, const std::string& path, bool isStatic = false) override;
    void LoadModelAsync(const std::string& name, const std::string& path, bool isStatic = false) override;
    void UnloadModel(const std::string& name) override;

    void LoadAnimation(const std::string& name, const std::string& path, const std::string& modelName);
    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize) override;
    void LoadSound(const std::string& name, const std::string& path) override;
    void LoadSkybox(const std::string& name, const std::vector<std::string>& faces) override;
    void LoadFragment(const std::string& name, const std::string& path);
    bool LoadVideo(const std::string& name, const std::string& path);
    void CreateUIModel(const std::string& name, UIType type) override;

    void UnloadShader(const std::string& name) override;
    void UnloadComputeShader(const std::string& name);
    void UnloadFont(const std::string& name) override;
    void UnloadSound(const std::string& name) override;
    void UnloadSkybox(const std::string& name) override;
    void UnloadAnimation(const std::string& name);
    void UnloadFragment(const std::string& name);
    void UnloadVideo(const std::string& name);

    std::shared_ptr<Shader> GetShader(const std::string& name) override;
    std::shared_ptr<ComputeShader> GetComputeShader(const std::string& name);
    std::shared_ptr<Texture> GetTexture(const std::string& name) override;
    std::shared_ptr<Model> GetModel(const std::string& name) override;
    std::shared_ptr<Animation> GetAnimation(const std::string& name);
    std::shared_ptr<Font> GetFont(const std::string& name) override;
    std::shared_ptr<IAudioSource> GetSound(const std::string& name) override;
    std::shared_ptr<Skybox> GetSkybox(const std::string& name) override;
    std::shared_ptr<FragmentAsset> GetFragment(const std::string& name);
    std::shared_ptr<UIModel> GetUIModel(const std::string& name) override;
    bool HasUIModel(const std::string& name);
    std::shared_ptr<VideoDecoder> GetVideo(const std::string& name);

    std::vector<std::string> GetLoadedTextures() const;
    std::vector<std::string> GetLoadedModels() const;
    std::vector<std::string> GetLoadedShaders() const;
    std::vector<std::string> GetLoadedComputeShaders() const;
    std::vector<std::string> GetLoadedSounds() const;
    std::vector<std::string> GetLoadedSkyboxes() const;
    std::vector<std::string> GetLoadedAnimations() const;
    std::vector<std::string> GetLoadedVideos() const;
    std::vector<std::string> GetLoadedFonts() const;
    std::vector<std::string> GetLoadedFragments() const;

    std::shared_ptr<Texture> GetTextureAuto(const std::string& nameOrPath);
    std::shared_ptr<Model> GetModelAuto(const std::string& nameOrPath, bool isStatic = false);
    std::shared_ptr<Font> GetFontAuto(const std::string& nameOrPath, unsigned int fontSize = 16);
    std::shared_ptr<IAudioSource> GetSoundAuto(const std::string& nameOrPath);

    void ClearResource();

    struct ResourceDefinition
    {
        std::string type;  // "Shader", "Texture", "Model", "Font", "Skybox", "Sound"
        std::string name;
        std::unordered_map<std::string, std::string> properties;
    };
    std::vector<ResourceDefinition> GetResourceDefinitions() const;
    void AddResourceDefinition(const std::string& type, const std::string& name,
                               const std::unordered_map<std::string, std::string>& props);
    bool ReimportResource(const std::string& name);

    void RegisterLoader(std::unique_ptr<ILoaderStrategy> strategy);
    bool LoadUnified(const std::string& type, const std::string& path);
    std::vector<std::string> GetRegisteredLoaderTypes() const;

private:
    friend void RegisterDefaultLoaderStrategies(ResourceManager& resourceManager);
    bool RegisterLoaderInternal(std::unique_ptr<ILoaderStrategy> strategy, bool replaceExisting);
    void SubscribeReloadEvents();
    void ReloadShader(const std::string& name);
    void ReloadComputeShader(const std::string& name);
    void ReloadTexture(const std::string& name);

    std::unique_ptr<ShaderManager> m_ShaderManager;
    IShaderManager* m_LowLevelShaderManager = nullptr;
    std::unordered_map<std::string, std::shared_ptr<ComputeShader>> m_ComputeShaders;
    std::unique_ptr<TextureManager> m_TextureManager;
    std::unique_ptr<ModelManager> m_ModelManager;
    std::unique_ptr<AudioAssetManager> m_AudioManager;
    std::unique_ptr<FontManager> m_FontManager;
    std::unique_ptr<SkyboxManager> m_SkyboxManager;
    std::unique_ptr<AnimationManager> m_AnimationManager;
    std::unique_ptr<VideoManager> m_VideoManager;
    std::unique_ptr<FragmentAssetManager> m_FragmentManager;
    std::unique_ptr<UIModelManager> m_UIModelManager;

    ResourceWatcher m_ResourceWatcher;

    mutable std::mutex m_ResourceMutex;

    std::unordered_map<std::string, std::shared_ptr<ILoaderStrategy>> m_Strategies;
    mutable std::shared_mutex m_StrategyMutex;

    std::vector<ResourceDefinition> m_ResourceDefinitions;

    bool m_HeadlessMode = false;
    bool m_StrictAssetLoading = false;
    int m_ReloadListenerId = -1;
    bool m_IsShutdown = false;
};
