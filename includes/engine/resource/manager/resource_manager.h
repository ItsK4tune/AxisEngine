#pragma once

#include <audio/interface/i_audio_engine.h>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <render/logic/animation.h>
#include <render/logic/font.h>
#include <render/logic/mesh.h>
#include <render/logic/model.h>
#include <render/logic/shader.h>
#include <render/logic/shader_cache.h>
#include <render/logic/skybox.h>
#include <render/logic/ui_model.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_font_library.h>
#include <resource/interface/i_model_library.h>
#include <resource/interface/i_shader_library.h>
#include <resource/interface/i_skybox_library.h>
#include <resource/interface/i_sound_library.h>
#include <resource/interface/i_texture_library.h>
#include <resource/logic/animation_cache.h>
#include <resource/logic/font_cache.h>
#include <resource/logic/resource_watcher.h>
#include <resource/logic/sound_cache.h>
#include <resource/logic/texture_cache.h>
#include <resource/manager/model_instance_manager.h>
#include <string>
#include <thread>
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

    void Initialize(IShaderManager& shaderManager);
    void Shutdown();
    void Update(float dt);

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

    // Auto-loading versions (Name or Path)
    std::shared_ptr<Texture> GetTextureAuto(const std::string& nameOrPath);
    std::shared_ptr<Model> GetModelAuto(const std::string& nameOrPath, bool isStatic = false);
    std::shared_ptr<Font> GetFontAuto(const std::string& nameOrPath, unsigned int fontSize = 16);
    std::shared_ptr<IAudioSource> GetSoundAuto(const std::string& nameOrPath, IAudioEngine* engine = nullptr);

    void ClearResource();

    ShaderCache* GetShaderCache() { return m_ShaderCache.get(); }
    ModelInstanceManager& GetModelInstanceManager() { return m_ModelInstanceManager; }

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
    void FlushPendingModels();

    struct PendingModel {
        std::string name;
        std::shared_ptr<Model> model;
    };

    IShaderManager* m_ShaderManager = nullptr;
    std::unique_ptr<ShaderCache> m_ShaderCache;
    ModelInstanceManager m_ModelInstanceManager;
    TextureCache m_TextureCache;
    FontCache m_FontCache;
    SoundCache m_SoundCache;
    AnimationCache m_AnimationCache;
    ResourceWatcher m_ResourceWatcher;

    struct ModelInfo {
        std::string path;
        bool isStatic;
    };
    std::unordered_map<std::string, ModelInfo> m_ModelPaths;
    std::unordered_map<std::string, std::shared_ptr<UIModel>> m_UIModels;
    std::unordered_map<std::string, std::shared_ptr<Skybox>> m_Skyboxes;

    std::unordered_map<std::string, std::string> m_PathToTextureName;
    std::unordered_map<std::string, std::string> m_PathToFontName;
    std::unordered_map<std::string, std::string> m_PathToModelName;
    std::unordered_map<std::string, std::string> m_PathToShaderName;
    std::unordered_map<std::string, std::string> m_PathToSoundName;
    std::unordered_map<std::string, std::string> m_PathToAnimationName;
    std::unordered_map<std::string, std::string> m_PathToSkyboxName;

    std::vector<PendingModel> m_PendingModels;
    std::vector<std::future<void>> m_ActiveFutures;
    std::mutex m_PendingMutex;
    mutable std::mutex m_ResourceMutex;

    int m_ReloadListenerId = -1;
};