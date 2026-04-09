#include <core/logic/job_system.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <iostream>
#include <resource/logic/resource_manager.h>
#include <resource/type/resource_events.h>
#include <core/logic/logger.h>
#include <core/logic/filesystem.h>
#include <core/logic/axis_assert.h>
#include <filesystem>
#include <fstream>
#include <sstream>

ResourceManager::ResourceManager()
{
    m_ReloadListenerId = EventManager::Instance().Subscribe<ResourceReloadEvent>([this](const ResourceReloadEvent &e)
                                                                                {
        if (e.type == "SHADER") {
            ReloadShader(e.name);
        } else if (e.type == "Texture") {
            ReloadTexture(e.name);
        } });
}

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
    m_FragmentManager = std::make_unique<FragmentAssetManager>();
    m_UIModelManager = std::make_unique<UIModelManager>();

    m_ShaderManager->Initialize();
    m_TextureManager->Initialize();
    // ModelManager::Initialize requires cubeModel from load.axs, so it should be called after LoadScene
}

void ResourceManager::InitializePostLoad()
{
    if (m_ModelManager) m_ModelManager->Initialize();
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::Shutdown()
{
    EventManager::Instance().Unsubscribe<ResourceReloadEvent>(m_ReloadListenerId);
    ClearResource();
}

void ResourceManager::SetTextureAsyncEnabled(bool enabled)
{
    if (m_TextureManager) m_TextureManager->SetAsyncEnabled(enabled);
}

void ResourceManager::SetTextureMaxAnisotropy(float max)
{
    if (m_TextureManager) m_TextureManager->SetMaxAnisotropy(max);
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
    
    auto shader = m_ShaderManager->Get(name);
    if (shader) {
        // Smart detection: check name OR content of the fragment shader
        bool isDeferred = (name.find("deferred") != std::string::npos || fsPath.find("deferred") != std::string::npos);
        if (!isDeferred) {
            std::ifstream f(fShaderPath);
            if (f.is_open()) {
                std::stringstream buffer;
                buffer << f.rdbuf();
                std::string fsContent = buffer.str();
                if (fsContent.find("gAlbedoSpec") != std::string::npos || fsContent.find("gPosition") != std::string::npos) {
                    isDeferred = true;
                }
            }
        }
        shader->SetDeferred(isDeferred);
    }

    m_ResourceWatcher.Watch(name, vShaderPath, "SHADER", vShaderPath, fShaderPath, gShaderPath);
}

void ResourceManager::LoadTexture(const std::string &name, const std::string &path, bool async, bool keepCpuData)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading Texture: " << name << " from " << path << (async ? " (async)" : "") << (keepCpuData ? " (keep CPU data)" : "");
    m_TextureManager->Load(name, fullPath, async, keepCpuData);

    m_ResourceWatcher.Watch(name, fullPath, "Texture");
}

void ResourceManager::LoadModel(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading model: " << name << " from " << fullPath;

    if (m_ModelManager->Load(name, fullPath, isStatic, false))
    {
        EventManager::Instance().Publish(ResourceLoadedEvent{name, "MODEL", true});
    }
}

void ResourceManager::LoadModelAsync(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Async loading model: " << name;

    m_ModelManager->Load(name, fullPath, isStatic, true);
}

void ResourceManager::LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading animation: " << name << " for model " << modelName;
    m_AnimationManager->Load(name, fullPath, modelName);
}

void ResourceManager::LoadFont(const std::string &name, const std::string &path, unsigned int fontSize)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading font: " << name << " (" << fontSize << "px) from " << path;
    m_FontManager->Load(name, fullPath, fontSize);
}

void ResourceManager::LoadSound(const std::string &name, const std::string &path, IAudioEngine *engine)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading sound: " << name;
    m_AudioManager->Load(name, fullPath);
}

void ResourceManager::LoadSkybox(const std::string &name, const std::vector<std::string> &faces)
{
    LOGGER_INFO("ResourceManager") << "Loading skybox: " << name;
    m_SkyboxManager->Load(name, faces);
}

void ResourceManager::LoadFragment(const std::string &name, const std::string &path)
{
    LOGGER_INFO("ResourceManager") << "Loading fragment: " << name << " from " << path;
    m_FragmentManager->Load(path);
}

void ResourceManager::CreateUIModel(const std::string &name, UIType type)
{
    m_UIModelManager->Create(name, type);
    LOGGER_INFO("ResourceManager") << "Created UI Model: " << name;
}

void ResourceManager::UnloadShader(const std::string &name) { m_ShaderManager->Unload(name); }
void ResourceManager::UnloadFont(const std::string &name) { m_FontManager->Unload(name); }
void ResourceManager::UnloadSound(const std::string &name) { m_AudioManager->Unload(name); }
void ResourceManager::UnloadSkybox(const std::string &name) { m_SkyboxManager->Unload(name); }
void ResourceManager::UnloadAnimation(const std::string &name) { m_AnimationManager->Unload(name); }
void ResourceManager::UnloadFragment(const std::string &name) { m_FragmentManager->Unload(name); }

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string &name) { return m_ShaderManager->Get(name); }
std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string &name) { return m_TextureManager->Get(name); }
std::shared_ptr<Model> ResourceManager::GetModel(const std::string &name) { return m_ModelManager->Get(name); }
std::shared_ptr<Animation> ResourceManager::GetAnimation(const std::string &name) { return m_AnimationManager->Get(name); }
std::shared_ptr<Font> ResourceManager::GetFont(const std::string &name) { return m_FontManager->Get(name); }
std::shared_ptr<IAudioSource> ResourceManager::GetSound(const std::string &name) { return m_AudioManager->Get(name); }
std::shared_ptr<Skybox> ResourceManager::GetSkybox(const std::string &name) { return m_SkyboxManager->Get(name); }
std::shared_ptr<FragmentAsset> ResourceManager::GetFragment(const std::string &name) { return m_FragmentManager->Load(name); }

std::shared_ptr<Texture> ResourceManager::GetTextureAuto(const std::string &nameOrPath)
{
    if (nameOrPath.empty()) return nullptr;
    auto t = GetTexture(nameOrPath);
    if (t) return t;
    
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
    if (nameOrPath.empty()) {
        if (nameOrPath != "time") return GetFontAuto("time", fontSize);
        return nullptr;
    }

    auto f = GetFont(nameOrPath);
    if (f) return f;
    
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) {
        if (nameOrPath != "time") return GetFontAuto("time", fontSize);
        return nullptr;
    }

    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) {
        if (nameOrPath != "time") return GetFontAuto("time", fontSize);
        return nullptr;
    }

    LoadFont(nameOrPath, nameOrPath, fontSize);
    auto res = GetFont(nameOrPath);
    if (res) return res;

    if (nameOrPath != "time") return GetFontAuto("time", fontSize);
    return nullptr;
}

std::shared_ptr<IAudioSource> ResourceManager::GetSoundAuto(const std::string &nameOrPath, IAudioEngine *engine)
{
    if (nameOrPath.empty()) return nullptr;
    auto s = GetSound(nameOrPath);
    if (s) return s;
    
    bool looksLikePath = nameOrPath.find('/') != std::string::npos ||
                         nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath) return nullptr;
    if (!std::filesystem::exists(FileSystem::getPath(nameOrPath))) return nullptr;
    LoadSound(nameOrPath, nameOrPath, engine);
    return GetSound(nameOrPath);
}

std::shared_ptr<UIModel> ResourceManager::GetUIModel(const std::string &name)
{
    return m_UIModelManager->Get(name);
}

bool ResourceManager::HasUIModel(const std::string &name)
{
    return m_UIModelManager->Has(name);
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
    if (m_FragmentManager) m_FragmentManager->Clear();

    if (m_UIModelManager) m_UIModelManager->Clear();

    LOGGER_INFO("ResourceManager") << "All resources cleared via specialized managers";
}
