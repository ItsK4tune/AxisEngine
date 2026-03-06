#pragma once

#include <future>
#include <rendering/core/shader.h>
#include <rendering/core/shader_cache.h>
#include <rendering/geometry/animation.h>
#include <rendering/geometry/mesh.h>
#include <rendering/geometry/model.h>
#include <rendering/renderer/font.h>
#include <rendering/renderer/skybox.h>
#include <rendering/renderer/ui_model.h>
#include <systems/audio/interfaces/i_audio_engine.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <resource/animation_cache.h>
#include <resource/font_cache.h>
#include <resource/model_instance_manager.h>
#include <resource/resource_watcher.h>
#include <resource/sound_cache.h>
#include <resource/texture_cache.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <resource/i_resource_libraries.h>

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

    void Init(IShaderManager& shaderManager);
    void Shutdown();
    void Update(float dt);

    void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "");
    void LoadTexture(const std::string& name, const std::string& path, bool async = true);
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

    void ClearResource();

    ShaderCache* GetShaderCache() { return m_ShaderCache.get(); }
    ModelInstanceManager& GetModelInstanceManager() { return m_ModelInstanceManager; }

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

    std::vector<PendingModel> m_PendingModels;
    std::vector<std::future<void>> m_ActiveFutures;
    std::mutex m_PendingMutex;
    mutable std::mutex m_ResourceMutex;

    int m_ReloadListenerId = -1;
};
