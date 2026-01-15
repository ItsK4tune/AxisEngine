#include <core/resource_manager.h>
#include <utils/filesystem.h>

#include <iostream>
#include <stb_image.h>
#include <vector>
#include <future>
#include <chrono>

ResourceManager::~ResourceManager()
{
    ClearResource();
}

void ResourceManager::Update()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_TextureFutures.begin();
        while (it != m_TextureFutures.end())
        {
            if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                TextureData data = it->get();
                if (data.data)
                {
                    unsigned int textureID;
                    glGenTextures(1, &textureID);

                    GLenum format = GL_RGBA;
                    if (data.nrComponents == 1)
                        format = GL_RED;
                    else if (data.nrComponents == 3)
                        format = GL_RGB;
                    else if (data.nrComponents == 4)
                        format = GL_RGBA;

                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.data);
                    glGenerateMipmap(GL_TEXTURE_2D);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    stbi_image_free(data.data);

                    Texture tex;
                    tex.id = textureID;
                    tex.type = "texture_diffuse";
                    tex.path = data.path;
                    textures[data.name] = tex;

                     std::cout << "[ResourceManager] Async Texture Loaded: " << data.name << "\n";
                }
                else {
                    std::cout << "[ResourceManager] Failed to async load texture: " << data.path << "\n";
                }
                it = m_TextureFutures.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    m_HotReloadTimer += 0.016f; 
    if (m_HotReloadTimer > 1.0f) {
        CheckHotReload();
        m_HotReloadTimer = 0.0f;
    }
}

void ResourceManager::CheckHotReload()
{
    for (auto& watcher : m_Watchers) {
        try {
            auto currentWriteTime = std::filesystem::last_write_time(watcher.filePath);
            if (currentWriteTime > watcher.lastWriteTime) {
                watcher.lastWriteTime = currentWriteTime;
                std::cout << "[HotReload] Detected change in: " << watcher.filePath << "\n";
                
                if (watcher.type == "SHADER") {
                    ReloadShader(watcher.resourceName);
                } else if (watcher.type == "TEXTURE") {
                    ReloadTexture(watcher.resourceName);
                }
            }

            if (watcher.type == "SHADER") {
                 if (!watcher.vsPath.empty()) {
                    auto vsTime = std::filesystem::last_write_time(watcher.vsPath);
                 }
            }
        } catch (const std::filesystem::filesystem_error& e) {
        }
    }
}

void ResourceManager::ReloadShader(const std::string& name)
{
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        std::string vs, fs, gs;
        for(const auto& w : m_Watchers) {
            if (w.resourceName == name && w.type == "SHADER") {
                vs = w.vsPath;
                fs = w.fsPath;
                gs = w.gsPath;
                break;
            }
        }
        
        if (!vs.empty() && !fs.empty()) {
            std::cout << "[HotReload] Reloading Shader: " << name << "\n";
            shaders[name] = std::make_unique<Shader>(vs.c_str(), fs.c_str(), gs.empty() ? nullptr : gs.c_str());
        }
    }
}

void ResourceManager::ReloadTexture(const std::string& name)
{
    std::string path;
    for(const auto& w : m_Watchers) {
        if (w.resourceName == name && w.type == "TEXTURE") {
            path = w.filePath;
            break;
        }
    }
    if(!path.empty()) {
        std::cout << "[HotReload] Reloading Texture: " << name << "\n";
        LoadTexture(name, path, false); 
    }
}

void ResourceManager::LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath, const std::string &gsPath)
{
    const char* gShaderPtr = nullptr;
    std::string gShaderPath;
    std::string vShaderPath = FileSystem::getPath(vsPath);
    std::string fShaderPath = FileSystem::getPath(fsPath);

    if (!gsPath.empty()) {
        gShaderPath = FileSystem::getPath(gsPath);
        gShaderPtr = gShaderPath.c_str();
    }
    shaders[name] = std::make_unique<Shader>(vShaderPath.c_str(), fShaderPath.c_str(), gShaderPtr);

    ResourceWatcher watcher;
    watcher.resourceName = name;
    watcher.type = "SHADER";
    watcher.vsPath = vShaderPath;
    watcher.fsPath = fShaderPath;
    watcher.gsPath = gShaderPath;

    watcher.filePath = vShaderPath; 
    try {
        watcher.lastWriteTime = std::filesystem::last_write_time(vShaderPath);
    } catch(...) {}
    m_Watchers.push_back(watcher);
}

void ResourceManager::LoadTexture(const std::string &name, const std::string &path, bool async)
{
    std::string fullPath = FileSystem::getPath(path);
    
    bool alreadyWatched = false;
    for(auto& w : m_Watchers) { if(w.resourceName == name && w.type == "TEXTURE") alreadyWatched = true; }
    if (!alreadyWatched) {
        ResourceWatcher watcher;
        watcher.resourceName = name;
        watcher.filePath = fullPath;
        watcher.type = "TEXTURE";
        try { watcher.lastWriteTime = std::filesystem::last_write_time(fullPath); } catch(...) {}
        m_Watchers.push_back(watcher);
    }


    if (async)
    {
        m_TextureFutures.push_back(std::async(std::launch::async, [name, fullPath]() -> TextureData {
            TextureData data;
            data.name = name;
            data.path = fullPath;
            stbi_set_flip_vertically_on_load(true);
            data.data = stbi_load(fullPath.c_str(), &data.width, &data.height, &data.nrComponents, 0);
            return data;
        }));
    }
    else
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); 
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            
            Texture tex;
            tex.id = textureID;
            tex.type = "texture_diffuse";
            tex.path = path;
            textures[name] = tex;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }
    }
}

Texture *ResourceManager::GetTexture(const std::string &name)
{
    if (textures.find(name) != textures.end())
        return &textures[name];
    return nullptr;
}

void ResourceManager::LoadModel(const std::string &name, const std::string &path, bool isStatic)
{
    models[name] = std::make_unique<Model>(FileSystem::getPath(path), isStatic);
}

void ResourceManager::LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName)
{
    Model *model = GetModel(modelName);
    if (model)
    {
        animations[name] = std::make_unique<Animation>(FileSystem::getPath(path), model);
    }
}

void ResourceManager::LoadFont(const std::string &name, const std::string &path, unsigned int fontSize)
{
    auto font = std::make_unique<Font>();
    if (font->Load(FileSystem::getPath(path), fontSize))
    {
        fonts[name] = std::move(font);
    }
}

void ResourceManager::LoadSound(const std::string &name, const std::string &path, irrklang::ISoundEngine *engine)
{
    if (!engine)
        return;

    std::string fullPath = FileSystem::getPath(path);

    irrklang::ISoundSource *source = engine->addSoundSourceFromFile(fullPath.c_str());

    if (source)
    {
        sounds[name] = source;
    }
    else
    {
        std::cerr << "[ResourceManager] Failed to load sound: " << fullPath << std::endl;
    }
}

void ResourceManager::LoadSkybox(const std::string &name, const std::vector<std::string> &faces)
{
    auto skybox = std::make_unique<Skybox>();
    skybox->LoadCubemap(faces);
    skyboxes[name] = std::move(skybox);
}

void ResourceManager::CreateUIModel(const std::string &name, UIType type)
{
    uiModels[name] = std::make_unique<UIModel>(type);
}

Shader *ResourceManager::GetShader(const std::string &name)
{
    if (shaders.find(name) != shaders.end())
        return shaders[name].get();
    std::cerr << "[ResourceManager] Shader not found: " << name << std::endl;
    return nullptr;
}

Model *ResourceManager::GetModel(const std::string &name)
{
    if (models.find(name) != models.end())
        return models[name].get();
    std::cerr << "[ResourceManager] Model not found: " << name << std::endl;
    return nullptr;
}

Animation *ResourceManager::GetAnimation(const std::string &name)
{
    if (animations.find(name) != animations.end())
        return animations[name].get();
    std::cerr << "[ResourceManager] Animation not found: " << name << std::endl;
    return nullptr;
}

Font *ResourceManager::GetFont(const std::string &name)
{
    if (fonts.find(name) != fonts.end())
        return fonts[name].get();
    std::cerr << "[ResourceManager] Font not found: " << name << std::endl;
    return nullptr;
}

UIModel *ResourceManager::GetUIModel(const std::string &name)
{
    if (uiModels.find(name) != uiModels.end())
        return uiModels[name].get();
    return nullptr;
}

irrklang::ISoundSource *ResourceManager::GetSound(const std::string &name)
{
    if (sounds.find(name) != sounds.end())
        return sounds[name];
    std::cerr << "[ResourceManager] Sound not found: " << name << std::endl;
    return nullptr;
}

Skybox *ResourceManager::GetSkybox(const std::string &name)
{
    if (skyboxes.find(name) != skyboxes.end())
        return skyboxes[name].get();
    std::cerr << "[ResourceManager] Skybox not found: " << name << std::endl;
    return nullptr;
}

void ResourceManager::ClearResource()
{
    shaders.clear();
    textures.clear();
    models.clear();
    animations.clear();
    fonts.clear();
    uiModels.clear();
    sounds.clear();
    skyboxes.clear();

    std::cout << "[ResourceManager] Cleared all resources.\n";
}