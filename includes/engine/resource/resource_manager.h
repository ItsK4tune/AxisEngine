#pragma once

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <irrKlang/irrKlang.h>

#include <graphic/core/shader.h>
#include <graphic/geometry/model.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/geometry/animation.h>
#include <graphic/renderer/font.h>
#include <graphic/renderer/skybox.h>
#include <graphic/geometry/mesh.h>
#include <graphic/core/shader_cache.h>
#include <resource/model_instance_manager.h>
#include <resource/texture_cache.h>
#include <resource/font_cache.h>
#include <resource/sound_cache.h>
#include <resource/animation_cache.h>
#include <resource/resource_watcher.h>

class ResourceManager
{
public:
    ~ResourceManager();

    void Update();

    void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "");
    void LoadTexture(const std::string& name, const std::string& path, bool async = true);
    void LoadModel(const std::string& name, const std::string& path, bool isStatic = false);
    void LoadAnimation(const std::string& name, const std::string& path, const std::string& modelName);
    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    void LoadSound(const std::string& name, const std::string& path, irrklang::ISoundEngine* engine);
    void LoadSkybox(const std::string& name, const std::vector<std::string>& faces);
    void CreateUIModel(const std::string& name, UIType type);

    Shader* GetShader(const std::string& name);
    Texture* GetTexture(const std::string& name);
    Model* GetModel(const std::string& name);
    Animation* GetAnimation(const std::string& name);
    Font* GetFont(const std::string& name);
    irrklang::ISoundSource* GetSound(const std::string& name);
    Skybox* GetSkybox(const std::string& name);
    UIModel* GetUIModel(const std::string& name);

    void ClearResource();
    
    ShaderCache& GetShaderCache() { return m_ShaderCache; }
    ModelInstanceManager& GetModelInstanceManager() { return m_ModelInstanceManager; }

private:
    void ReloadShader(const std::string& name);
    void ReloadTexture(const std::string& name);

    ShaderCache m_ShaderCache;
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
    std::map<std::string, std::unique_ptr<UIModel>> m_UIModels;
    std::map<std::string, std::unique_ptr<Skybox>> m_Skyboxes;
};