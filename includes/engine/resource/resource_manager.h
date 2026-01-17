#pragma once

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <future>
#include <mutex>
#include <filesystem>
#include <irrKlang/irrKlang.h>

#include <graphic/shader.h>
#include <graphic/model.h>
#include <graphic/ui_model.h>
#include <graphic/animation.h>
#include <graphic/font.h>
#include <graphic/font.h>
#include <graphic/skybox.h>
#include <graphic/mesh.h>

struct TextureData {
    std::string name;
    std::string path;
    int width, height, nrComponents;
    unsigned char* data = nullptr;
};

struct ResourceWatcher {
    std::string resourceName;
    std::string filePath;
    std::filesystem::file_time_type lastWriteTime;
    std::string type;
    std::string vsPath;
    std::string fsPath;
    std::string gsPath;
};

class ResourceManager
{
public:
    ~ResourceManager();

    void Update();

    void LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath, const std::string &gsPath = "");
    void LoadTexture(const std::string &name, const std::string &path, bool async = true);
    void LoadModel(const std::string &name, const std::string &path, bool isStatic = false);
    void LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName);
    void LoadFont(const std::string &name, const std::string &path, unsigned int fontSize);
    void LoadSound(const std::string &name, const std::string &path, irrklang::ISoundEngine *engine);
    void LoadSkybox(const std::string &name, const std::vector<std::string> &faces);
    void CreateUIModel(const std::string &name, UIType type);

    Shader *GetShader(const std::string &name);
    Texture *GetTexture(const std::string &name);
    Model *GetModel(const std::string &name);
    Animation *GetAnimation(const std::string &name);
    Font *GetFont(const std::string &name);
    irrklang::ISoundSource *GetSound(const std::string &name);
    Skybox *GetSkybox(const std::string &name);
    UIModel *GetUIModel(const std::string &name);

    void ClearResource();

private:
    void CheckHotReload();
    void ReloadShader(const std::string& name);
    void ReloadTexture(const std::string& name);

    std::map<std::string, std::unique_ptr<Shader>> shaders;
    std::map<std::string, Texture> textures;
    std::map<std::string, std::unique_ptr<Model>> models;
    std::map<std::string, std::unique_ptr<Animation>> animations;
    std::map<std::string, std::unique_ptr<Font>> fonts;
    std::map<std::string, std::unique_ptr<UIModel>> uiModels;
    std::map<std::string, irrklang::ISoundSource *> sounds;
    std::map<std::string, std::unique_ptr<Skybox>> skyboxes;

    std::vector<std::future<TextureData>> m_TextureFutures;
    std::vector<ResourceWatcher> m_Watchers;
    std::mutex m_Mutex;
    float m_HotReloadTimer = 0.0f;
};