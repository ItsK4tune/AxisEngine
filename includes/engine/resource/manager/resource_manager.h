#pragma once

#include <render/interface/i_shader_manager.h>
#include <resource/manager/audio_asset_manager.h>
#include <resource/manager/shader_manager.h>
#include <resource/manager/texture_manager.h>
#include <resource/manager/model_manager.h>
#include <resource/manager/video_manager.h>
#include <resource/manager/font_manager.h>
#include <resource/manager/skybox_manager.h>
#include <resource/manager/animation_manager.h>
#include <resource/logic/resource_watcher.h>
#include <resource/manager/model_instance_manager.h>
#include <resource/interface/i_resource_libraries.h>
#include <render/logic/ui_model.h>
#include <mutex>
#include <future>
#include <string>
#include <memory>
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

    /**
     * @brief Initializes the facade with necessary low-level dependencies.
     */
    void Initialize(IShaderManager& shaderManager, ITextureManager& textureManager, IAudioEngine& audioEngine);
    void Shutdown();
    void Update(float dt);

    // Facade Methods (delegating to specialized managers)
    void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "");
    void LoadTexture(const std::string& name, const std::string& path, bool async = true, bool keepCpuData = false) override;
    void UnloadTexture(const std::string& name);

    void LoadModel(const std::string& name, const std::string& path, bool isStatic = false);
    void LoadModelAsync(const std::string& name, const std::string& path, bool isStatic = false);
    void UnloadModel(const std::string& name);

    void LoadAnimation(const std::string& name, const std::string& path, const std::string& modelName);
    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine);
    void LoadSkybox(const std::string& name, const std::vector<std::string>& faces);
    void CreateUIModel(const std::string& name, UIType type);

    void UnloadShader(const std::string& name);
    void UnloadFont(const std::string& name);
    void UnloadSound(const std::string& name);
    void UnloadSkybox(const std::string& name);
    void UnloadAnimation(const std::string& name);

    std::shared_ptr<Shader> GetShader(const std::string& name);
    std::shared_ptr<Texture> GetTexture(const std::string& name);
    std::shared_ptr<Model> GetModel(const std::string& name);
    std::shared_ptr<Animation> GetAnimation(const std::string& name);
    std::shared_ptr<Font> GetFont(const std::string& name);
    std::shared_ptr<IAudioSource> GetSound(const std::string& name);
    std::shared_ptr<Skybox> GetSkybox(const std::string& name);
    std::shared_ptr<UIModel> GetUIModel(const std::string& name);
    bool HasUIModel(const std::string& name);

    // Auto-loading versions (Helper methods)
    std::shared_ptr<Texture> GetTextureAuto(const std::string& nameOrPath);
    std::shared_ptr<Model> GetModelAuto(const std::string& nameOrPath, bool isStatic = false);
    std::shared_ptr<Font> GetFontAuto(const std::string& nameOrPath, unsigned int fontSize = 16);
    std::shared_ptr<IAudioSource> GetSoundAuto(const std::string& nameOrPath, IAudioEngine* engine = nullptr);

    void ClearResource();

    // Accessors for specialized managers (Service Locator pattern)
    ShaderManager& GetShaderManager()    { return *m_ShaderManager; }
    TextureManager& GetTextureManager()  { return *m_TextureManager; }
    ModelManager& GetModelManager()      { return *m_ModelManager; }
    AudioAssetManager& GetAudioManager()      { return *m_AudioManager; }
    FontManager& GetFontManager()        { return *m_FontManager; }
    SkyboxManager& GetSkyboxManager()    { return *m_SkyboxManager; }
    AnimationManager& GetAnimationManager() { return *m_AnimationManager; }
    VideoManager& GetVideoManager()      { return *m_VideoManager; }

    ModelInstanceManager& GetModelInstanceManager() { return m_ModelInstanceManager; }

    // Path helpers
    std::string GetTextureNameFromPath(const std::string& path);
    std::string GetFontNameFromPath(const std::string& path);
    std::string GetModelNameFromPath(const std::string& path);
    std::string GetShaderNameFromPath(const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "");
    std::string GetSoundNameFromPath(const std::string& path);
    std::string GetAnimationNameFromPath(const std::string& path);
    std::string GetSkyboxNameFromPaths(const std::vector<std::string>& faces);

private:
    void ReloadShader(const std::string& name);
    void ReloadTexture(const std::string& name);

    // Specialized Managers
    std::unique_ptr<ShaderManager>    m_ShaderManager;
    std::unique_ptr<TextureManager>   m_TextureManager;
    std::unique_ptr<ModelManager>     m_ModelManager;
    std::unique_ptr<AudioAssetManager> m_AudioManager;
    std::unique_ptr<FontManager>      m_FontManager;
    std::unique_ptr<SkyboxManager>    m_SkyboxManager;
    std::unique_ptr<AnimationManager> m_AnimationManager;
    std::unique_ptr<VideoManager>     m_VideoManager;

    ModelInstanceManager m_ModelInstanceManager;
    ResourceWatcher      m_ResourceWatcher;

    std::mutex           m_ResourceMutex;

    std::unordered_map<std::string, std::shared_ptr<UIModel>> m_UIModels;

    // Name-to-path and path-to-name mappings (kept for facade convenience)
    std::unordered_map<std::string, std::string> m_PathToTextureName;
    std::unordered_map<std::string, std::string> m_PathToFontName;
    std::unordered_map<std::string, std::string> m_PathToModelName;
    std::unordered_map<std::string, std::string> m_PathToShaderName;
    std::unordered_map<std::string, std::string> m_PathToSoundName;
    std::unordered_map<std::string, std::string> m_PathToAnimationName;
    std::unordered_map<std::string, std::string> m_PathToSkyboxName;

    int m_ReloadListenerId = -1;
};
