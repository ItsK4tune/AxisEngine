#include <core/logic/job_system.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <iostream>
#include <resource/logic/resource_manager.h>
#include <core/logic/logger.h>
#include <core/logic/filesystem.h>
#include <core/logic/axis_assert.h>
#include <filesystem>

ResourceManager::ResourceManager()
{
    m_ReloadListenerId = EventSystem::Instance().Subscribe<ResourceReloadEvent>([this](const ResourceReloadEvent &e)
                                                                                {
        if (e.type == "SHADER") {
            ReloadShader(e.name);
        } else if (e.type == "Texture") {
            ReloadTexture(e.name);
        } });
}

// --- Unified Loading (merged from UnifiedLoader) ---

void ResourceManager::RegisterLoader(std::unique_ptr<ILoaderStrategy> strategy)
{
    if (!strategy) return;
    std::string name = strategy->GetName();
    m_Strategies[name] = std::move(strategy);
    LOGGER_INFO("ResourceManager") << "Registered loading strategy: " << name;
}

bool ResourceManager::LoadUnified(const std::string& type, const std::string& path)
{
    auto it = m_Strategies.find(type);
    if (it != m_Strategies.end()) {
        LOGGER_INFO("ResourceManager") << "Dispatching '" << path << "' to strategy: " << type;
        return it->second->Load(path);
    }
    
    LOGGER_ERROR("ResourceManager") << "No strategy found for type: " << type;
    return false;
}

std::vector<std::string> ResourceManager::GetRegisteredLoaderTypes() const
{
    std::vector<std::string> types;
    for (const auto& pair : m_Strategies) {
        types.push_back(pair.first);
    }
    return types;
}

void ResourceManager::Initialize(IShaderManager& shaderManager, ITextureManager& textureManager, IAudioEngine& audioEngine)
{
    m_ShaderManager = std::make_unique<ShaderManager>(shaderManager);
    m_TextureManager = std::make_unique<TextureManager>(textureManager);
    m_ModelManager = std::make_unique<ModelManager>(m_ModelInstanceManager);
    m_AudioManager = std::make_unique<AudioAssetManager>(audioEngine);
    m_FontManager = std::make_unique<FontManager>();
    m_SkyboxManager = std::make_unique<SkyboxManager>();
    m_AnimationManager = std::make_unique<AnimationManager>(*m_ModelManager);
    m_VideoManager = std::make_unique<VideoManager>();
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::Shutdown()
{
    EventSystem::Instance().Unsubscribe<ResourceReloadEvent>(m_ReloadListenerId);
    ClearResource();
}

void ResourceManager::Update(float dt)
{
    m_TextureManager->Update();
    m_ModelManager->Update();
    m_ResourceWatcher.Update(dt);
}

void ResourceManager::UnloadTexture(const std::string &name)
{
    m_TextureManager->Unload(name);
}

void ResourceManager::UnloadModel(const std::string &name)
{
    m_ModelManager->Unload(name);
}

void ResourceManager::ReloadShader(const std::string &name)
{
    LOGGER_INFO("HotReload") << "Reloading Shader: " << name;
    m_ShaderManager->Reload(name);
}

void ResourceManager::ReloadTexture(const std::string &name)
{
    LOGGER_INFO("HotReload") << "Reloading Texture: " << name;
}

void ResourceManager::LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath, const std::string &gsPath)
{
    std::string vShaderPath = FileSystem::getPath(vsPath);
    std::string fShaderPath = FileSystem::getPath(fsPath);
    std::string gShaderPath = gsPath.empty() ? "" : FileSystem::getPath(gsPath);

    LOGGER_INFO("ResourceManager") << "Loading shader: " << name << " (" << vsPath << ", " << fsPath << ")";
    m_ShaderManager->Load(name, vShaderPath, fShaderPath, gShaderPath);

    m_ResourceWatcher.Watch(name, vShaderPath, "SHADER", vShaderPath, fShaderPath, gShaderPath);

    std::string shaderKey = vShaderPath + "|" + fShaderPath + "|" + gShaderPath;
    m_PathToShaderName[shaderKey] = name;
}

void ResourceManager::LoadTexture(const std::string &name, const std::string &path, bool async, bool keepCpuData)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading Texture: " << name << " from " << path << (async ? " (async)" : "") << (keepCpuData ? " (keep CPU data)" : "");
    m_TextureManager->Load(name, fullPath, async, keepCpuData);

    m_ResourceWatcher.Watch(name, fullPath, "Texture");
    m_PathToTextureName[fullPath] = name;
}

void ResourceManager::LoadModel(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading model: " << name << " from " << fullPath;

    if (m_ModelManager->Load(name, fullPath, isStatic, false))
    {
        m_PathToModelName[fullPath] = name;
        EventSystem::Instance().Publish(ResourceLoadedEvent{name, "MODEL", true});
    }
}

void ResourceManager::LoadModelAsync(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Async loading model: " << name;

    m_ModelManager->Load(name, fullPath, isStatic, true);
    m_PathToModelName[fullPath] = name;
}

void ResourceManager::LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading animation: " << name << " for model " << modelName;
    m_AnimationManager->Load(name, fullPath, modelName);
    m_PathToAnimationName[fullPath] = name;
}

void ResourceManager::LoadFont(const std::string &name, const std::string &path, unsigned int fontSize)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading font: " << name << " (" << fontSize << "px) from " << path;
    m_FontManager->Load(name, fullPath, fontSize);
    m_PathToFontName[fullPath] = name;
}

void ResourceManager::LoadSound(const std::string &name, const std::string &path, IAudioEngine *engine)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading sound: " << name;
    m_AudioManager->Load(name, fullPath);
    m_PathToSoundName[fullPath] = name;
}

void ResourceManager::LoadSkybox(const std::string &name, const std::vector<std::string> &faces)
{
    LOGGER_INFO("ResourceManager") << "Loading skybox: " << name;
    m_SkyboxManager->Load(name, faces);
    
    std::string combinedPaths = "";
    for (const auto &face : faces) combinedPaths += FileSystem::getPath(face) + "|";
    m_PathToSkyboxName[combinedPaths] = name;
}

void ResourceManager::CreateUIModel(const std::string &name, UIType type)
{
    m_UIModels[name] = std::make_shared<UIModel>(type);
    LOGGER_INFO("ResourceManager") << "Created UI Model: " << name;
}

void ResourceManager::UnloadShader(const std::string &name) { m_ShaderManager->Unload(name); }
void ResourceManager::UnloadFont(const std::string &name) { m_FontManager->Unload(name); }
void ResourceManager::UnloadSound(const std::string &name) { m_AudioManager->Unload(name); }
void ResourceManager::UnloadSkybox(const std::string &name) { m_SkyboxManager->Unload(name); }
void ResourceManager::UnloadAnimation(const std::string &name) { m_AnimationManager->Unload(name); }

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string &name) { return m_ShaderManager->Get(name); }
std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string &name) { return m_TextureManager->Get(name); }
std::shared_ptr<Model> ResourceManager::GetModel(const std::string &name) { return m_ModelManager->Get(name); }
std::shared_ptr<Animation> ResourceManager::GetAnimation(const std::string &name) { return m_AnimationManager->Get(name); }
std::shared_ptr<Font> ResourceManager::GetFont(const std::string &name) { return m_FontManager->Get(name); }
std::shared_ptr<IAudioSource> ResourceManager::GetSound(const std::string &name) { return m_AudioManager->Get(name); }

std::shared_ptr<Texture> ResourceManager::GetTextureAuto(const std::string &nameOrPath)
{
    if (nameOrPath.empty()) return nullptr;
    auto t = GetTexture(nameOrPath);
    if (t) return t;
    std::string name = GetTextureNameFromPath(nameOrPath);
    if (!name.empty()) return GetTexture(name);
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) return nullptr;
    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) return nullptr;
    LoadTexture(nameOrPath, nameOrPath);
    return GetTexture(nameOrPath);
}

std::shared_ptr<Model> ResourceManager::GetModelAuto(const std::string &nameOrPath, bool isStatic)
{
    if (nameOrPath.empty()) return nullptr;
    auto m = GetModel(nameOrPath);
    if (m) return m;
    std::string name = GetModelNameFromPath(nameOrPath);
    if (!name.empty()) return GetModel(name);
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) return nullptr;
    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) return nullptr;
    LoadModel(nameOrPath, nameOrPath, isStatic);
    return GetModel(nameOrPath);
}

std::shared_ptr<Font> ResourceManager::GetFontAuto(const std::string &nameOrPath, unsigned int fontSize)
{
    if (nameOrPath.empty()) return nullptr;
    auto f = GetFont(nameOrPath);
    if (f) return f;
    std::string name = GetFontNameFromPath(nameOrPath);
    if (!name.empty()) return GetFont(name);
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) return nullptr;
    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) return nullptr;
    LoadFont(nameOrPath, nameOrPath, fontSize);
    return GetFont(nameOrPath);
}

std::shared_ptr<IAudioSource> ResourceManager::GetSoundAuto(const std::string &nameOrPath, IAudioEngine *engine)
{
    if (nameOrPath.empty()) return nullptr;
    auto s = GetSound(nameOrPath);
    if (s) return s;
    std::string name = GetSoundNameFromPath(nameOrPath);
    if (!name.empty()) return GetSound(name);
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) return nullptr;
    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) return nullptr;
    LoadSound(nameOrPath, nameOrPath, engine);
    return GetSound(nameOrPath);
}

std::shared_ptr<Skybox> ResourceManager::GetSkybox(const std::string &name)
{
    return m_SkyboxManager->Get(name);
}

std::shared_ptr<UIModel> ResourceManager::GetUIModel(const std::string &name)
{
    if (m_UIModels.find(name) != m_UIModels.end())
        return m_UIModels[name];
    return nullptr;
}

bool ResourceManager::HasUIModel(const std::string &name)
{
    return m_UIModels.find(name) != m_UIModels.end();
}

std::string ResourceManager::GetTextureNameFromPath(const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    auto it = m_PathToTextureName.find(fullPath);
    return (it != m_PathToTextureName.end()) ? it->second : "";
}

std::string ResourceManager::GetFontNameFromPath(const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToFontName.find(fullPath);
    if (it != m_PathToFontName.end())
        return it->second;
    return "";
}

std::string ResourceManager::GetModelNameFromPath(const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToModelName.find(fullPath);
    if (it != m_PathToModelName.end())
        return it->second;
    return "";
}

std::string ResourceManager::GetShaderNameFromPath(const std::string &vsPath, const std::string &fsPath, const std::string &gsPath)
{
    std::string vShaderPath = FileSystem::getPath(vsPath);
    std::string fShaderPath = FileSystem::getPath(fsPath);
    std::string gShaderPath = gsPath.empty() ? "" : FileSystem::getPath(gsPath);
    std::string shaderKey = vShaderPath + "|" + fShaderPath + "|" + gShaderPath;

    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToShaderName.find(shaderKey);
    if (it != m_PathToShaderName.end())
        return it->second;
    return "";
}

std::string ResourceManager::GetSoundNameFromPath(const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToSoundName.find(fullPath);
    if (it != m_PathToSoundName.end())
        return it->second;
    return "";
}

std::string ResourceManager::GetAnimationNameFromPath(const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToAnimationName.find(fullPath);
    if (it != m_PathToAnimationName.end())
        return it->second;
    return "";
}

std::string ResourceManager::GetSkyboxNameFromPaths(const std::vector<std::string> &faces)
{
    std::string combinedPaths = "";
    for (const auto &face : faces)
    {
        combinedPaths += FileSystem::getPath(face) + "|";
    }
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_PathToSkyboxName.find(combinedPaths);
    if (it != m_PathToSkyboxName.end())
        return it->second;
    return "";
}
void ResourceManager::ClearResource()
{
    m_ShaderManager->Clear();
    m_TextureManager->Clear();
    m_ModelManager->Clear();
    m_AudioManager->Clear();
    m_FontManager->Clear();
    m_AnimationManager->Clear();
    m_SkyboxManager->Clear();
    m_VideoManager->Clear();

    m_UIModels.clear();
    m_PathToTextureName.clear();
    m_PathToFontName.clear();
    m_PathToModelName.clear();
    m_PathToShaderName.clear();
    m_PathToSoundName.clear();
    m_PathToAnimationName.clear();
    m_PathToSkyboxName.clear();

    LOGGER_INFO("ResourceManager") << "All resources cleared via specialized managers";
}