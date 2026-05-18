#include <resource/logic/resource_manager.h>
#include <audio/interface/i_audio_source.h>
#include <core/logic/axis_assert.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <resource/type/resource_events.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

ResourceManager::ResourceManager()
{
    m_ReloadListenerId = EventManager::Instance().Subscribe<ResourceReloadEvent>([this](const ResourceReloadEvent& e) {
        if (e.type == "SHADER")
        {
            ReloadShader(e.name);
        }
        else if (e.type == "Texture")
        {
            ReloadTexture(e.name);
        }
    });
}

void ResourceManager::RegisterLoader(std::unique_ptr<ILoaderStrategy> strategy)
{
    if (!strategy)
        return;
    std::string name = strategy->GetName();
    m_Strategies[name] = std::move(strategy);
    LOGGER_INFO("ResourceManager") << "Registered loading strategy: " << name;
}

bool ResourceManager::LoadUnified(const std::string& type, const std::string& path)
{
    auto it = m_Strategies.find(type);
    if (it != m_Strategies.end())
    {
        LOGGER_INFO("ResourceManager") << "Dispatching '" << path << "' to strategy: " << type;
        return it->second->Load(path);
    }

    LOGGER_ERROR("ResourceManager") << "No strategy found for type: " << type;
    return false;
}

std::vector<std::string> ResourceManager::GetRegisteredLoaderTypes() const
{
    std::vector<std::string> types;
    for (const auto& pair : m_Strategies)
    {
        types.push_back(pair.first);
    }
    return types;
}

void ResourceManager::Initialize(IShaderManager* shaderManager, ITextureManager* textureManager,
                                 IAudioEngine& audioEngine)
{
    m_HeadlessMode = (shaderManager == nullptr && textureManager == nullptr);

    if (shaderManager)
        m_ShaderManager = std::make_unique<ShaderManager>(*shaderManager);
    if (textureManager)
        m_TextureManager = std::make_unique<TextureManager>(*textureManager);

    m_ModelManager = std::make_unique<ModelManager>(m_ModelInstanceManager);
    m_AudioManager = std::make_unique<AudioAssetManager>(audioEngine);
    m_FontManager = std::make_unique<FontManager>();
    m_SkyboxManager = std::make_unique<SkyboxManager>();
    m_AnimationManager = std::make_unique<AnimationManager>(*m_ModelManager);
    m_VideoManager = std::make_unique<VideoManager>();
    m_FragmentManager = std::make_unique<FragmentAssetManager>();
    m_UIModelManager = std::make_unique<UIModelManager>();

    if (m_ShaderManager)
        m_ShaderManager->Initialize();
    if (m_TextureManager)
        m_TextureManager->Initialize();
    // ModelManager::Initialize requires cubeModel from load.axs, so it should be called after LoadScene
}

void ResourceManager::InitializeHeadless()
{
    m_HeadlessMode = true;
    m_ModelManager = std::make_unique<ModelManager>(m_ModelInstanceManager);
    m_FragmentManager = std::make_unique<FragmentAssetManager>();
    m_AnimationManager = std::make_unique<AnimationManager>(*m_ModelManager);
    // No ShaderManager, TextureManager, AudioManager, FontManager, SkyboxManager, VideoManager, UIModelManager in pure
    LOGGER_INFO("ResourceManager") << "Initialized in HEADLESS mode (simulation resources only)";
}

void ResourceManager::InitializePostLoad()
{
    if (m_ModelManager)
        m_ModelManager->Initialize();
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
    if (m_TextureManager)
        m_TextureManager->SetAsyncEnabled(enabled);
}

void ResourceManager::SetTextureMaxAnisotropy(float max)
{
    if (m_TextureManager)
        m_TextureManager->SetMaxAnisotropy(max);
}

void ResourceManager::Update(float dt)
{
    if (m_TextureManager)
        m_TextureManager->Update();
    if (m_ModelManager)
        m_ModelManager->Update();
    m_ResourceWatcher.Update(dt);
}

void ResourceManager::UnloadTexture(const std::string& name)
{
    if (m_TextureManager)
        m_TextureManager->Unload(name);
}

void ResourceManager::UnloadModel(const std::string& name)
{
    if (m_ModelManager)
        m_ModelManager->Unload(name);
}

void ResourceManager::ReloadShader(const std::string& name)
{
    if (m_ShaderManager)
    {
        LOGGER_INFO("HotReload") << "Reloading Shader: " << name;
        m_ShaderManager->Reload(name);
    }
}

void ResourceManager::ReloadTexture(const std::string& name)
{
    LOGGER_INFO("HotReload") << "Reloading Texture: " << name;
}

void ResourceManager::AddResourceDefinition(const std::string& type, const std::string& name,
                                            const std::unordered_map<std::string, std::string>& props)
{
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    for (auto& def : m_ResourceDefinitions)
    {
        if (def.type == type && def.name == name)
        {
            def.properties = props;
            return;
        }
    }
    m_ResourceDefinitions.push_back({type, name, props});
}

void ResourceManager::LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath,
                                 const std::string& gsPath)
{
    if (!m_ShaderManager)
        return;

    std::string vShaderPath = FileSystem::getPath(vsPath);
    std::string fShaderPath = FileSystem::getPath(fsPath);
    std::string gShaderPath = gsPath.empty() ? "" : FileSystem::getPath(gsPath);

    LOGGER_INFO("ResourceManager") << "Loading shader: " << name << " (" << vsPath << ", " << fsPath << ")";
    m_ShaderManager->Load(name, vShaderPath, fShaderPath, gShaderPath);

    auto shader = m_ShaderManager->Get(name);
    if (shader)
    {
        // Smart detection: check name OR content of the fragment shader
        bool isDeferred = (name.find("deferred") != std::string::npos || fsPath.find("deferred") != std::string::npos);
        if (!isDeferred)
        {
            std::ifstream f(fShaderPath);
            if (f.is_open())
            {
                std::stringstream buffer;
                buffer << f.rdbuf();
                std::string fsContent = buffer.str();
                if (fsContent.find("gAlbedoSpec") != std::string::npos ||
                    fsContent.find("gPosition") != std::string::npos)
                {
                    isDeferred = true;
                }
            }
        }
        shader->SetDeferred(isDeferred);
    }

    m_ResourceWatcher.Watch(name, vShaderPath, "SHADER", vShaderPath, fShaderPath, gShaderPath);

    std::unordered_map<std::string, std::string> props = {{"Vertex", vsPath}, {"Fragment", fsPath}};
    if (!gsPath.empty())
        props["Geometry"] = gsPath;
    AddResourceDefinition("Shader", name, props);
}

void ResourceManager::LoadTexture(const std::string& name, const std::string& path, bool async, bool keepCpuData)
{
    if (m_HeadlessMode || !m_TextureManager)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading Texture: " << name << " from " << path << (async ? " (async)" : "")
                                   << (keepCpuData ? " (keep CPU data)" : "");
    m_TextureManager->Load(name, fullPath, async, keepCpuData);

    m_ResourceWatcher.Watch(name, fullPath, "Texture");
    AddResourceDefinition("Texture", name, {{"Path", path}});
}

void ResourceManager::LoadModel(const std::string& name, const std::string& path, bool isStatic)
{
    if (m_HeadlessMode)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading model: " << name;

    if (m_ModelManager->Load(name, fullPath, isStatic, false))
    {
        EventManager::Instance().Publish(ResourceLoadedEvent{name, "MODEL", true});
        AddResourceDefinition("Model", name, {{"Path", path}, {"Static", isStatic ? "1" : "0"}});
    }
}

void ResourceManager::LoadModelAsync(const std::string& name, const std::string& path, bool isStatic)
{
    if (m_HeadlessMode)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Async loading model: " << name;

    m_ModelManager->Load(name, fullPath, isStatic, true);
    AddResourceDefinition("Model", name, {{"Path", path}, {"Static", isStatic ? "1" : "0"}});
}

void ResourceManager::LoadAnimation(const std::string& name, const std::string& path, const std::string& modelName)
{
    if (m_HeadlessMode)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading animation: " << name << " for model " << modelName;
    m_AnimationManager->Load(name, fullPath, modelName);

    AddResourceDefinition("Animation", name, {{"Path", path}, {"Model", modelName}});
}

void ResourceManager::LoadFont(const std::string& name, const std::string& path, unsigned int fontSize)
{
    if (m_HeadlessMode || !m_FontManager)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading font: " << name << " (" << fontSize << "px) from " << path;
    m_FontManager->Load(name, fullPath, fontSize);

    AddResourceDefinition("Font", name, {{"Path", path}, {"Size", std::to_string(fontSize)}});
}

void ResourceManager::LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine)
{
    if (!m_AudioManager)
        return;
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading sound: " << name;
    m_AudioManager->Load(name, fullPath);

    AddResourceDefinition("Sound", name, {{"Path", path}});
}

void ResourceManager::LoadSkybox(const std::string& name, const std::vector<std::string>& faces)
{
    if (m_HeadlessMode || !m_SkyboxManager)
        return;
    LOGGER_INFO("ResourceManager") << "Loading skybox: " << name;
    m_SkyboxManager->Load(name, faces);

    std::unordered_map<std::string, std::string> props;
    if (faces.size() >= 6)
    {
        props["Right"] = faces[0];
        props["Left"] = faces[1];
        props["Top"] = faces[2];
        props["Bottom"] = faces[3];
        props["Front"] = faces[4];
        props["Back"] = faces[5];
    }
    AddResourceDefinition("Skybox", name, props);
}

void ResourceManager::LoadFragment(const std::string& name, const std::string& path)
{
    LOGGER_INFO("ResourceManager") << "Loading fragment: " << name << " from " << path;
    m_FragmentManager->Load(path);
}

void ResourceManager::CreateUIModel(const std::string& name, UIType type)
{
    if (m_HeadlessMode || !m_UIModelManager)
        return;
    m_UIModelManager->Create(name, type);
    LOGGER_INFO("ResourceManager") << "Created UI Model: " << name;
}

void ResourceManager::UnloadShader(const std::string& name)
{
    if (m_ShaderManager)
        m_ShaderManager->Unload(name);
}
void ResourceManager::UnloadFont(const std::string& name)
{
    if (m_FontManager)
        m_FontManager->Unload(name);
}
void ResourceManager::UnloadSound(const std::string& name)
{
    if (m_AudioManager)
        m_AudioManager->Unload(name);
}
void ResourceManager::UnloadSkybox(const std::string& name)
{
    if (m_SkyboxManager)
        m_SkyboxManager->Unload(name);
}
void ResourceManager::UnloadAnimation(const std::string& name)
{
    if (m_AnimationManager)
        m_AnimationManager->Unload(name);
}
void ResourceManager::UnloadFragment(const std::string& name)
{
    if (m_FragmentManager)
        m_FragmentManager->Unload(name);
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name)
{
    return m_ShaderManager ? m_ShaderManager->Get(name) : nullptr;
}
std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& name)
{
    return m_TextureManager ? m_TextureManager->Get(name) : nullptr;
}
std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name)
{
    return m_ModelManager ? m_ModelManager->Get(name) : nullptr;
}
std::shared_ptr<Animation> ResourceManager::GetAnimation(const std::string& name)
{
    return m_AnimationManager ? m_AnimationManager->Get(name) : nullptr;
}
std::shared_ptr<Font> ResourceManager::GetFont(const std::string& name)
{
    return m_FontManager ? m_FontManager->Get(name) : nullptr;
}
std::shared_ptr<IAudioSource> ResourceManager::GetSound(const std::string& name)
{
    return m_AudioManager ? m_AudioManager->Get(name) : nullptr;
}
std::shared_ptr<Skybox> ResourceManager::GetSkybox(const std::string& name)
{
    return m_SkyboxManager ? m_SkyboxManager->Get(name) : nullptr;
}
std::shared_ptr<FragmentAsset> ResourceManager::GetFragment(const std::string& name)
{
    return m_FragmentManager ? m_FragmentManager->Load(name) : nullptr;
}
std::shared_ptr<VideoDecoder> ResourceManager::GetVideo(const std::string& name)
{
    return m_VideoManager ? m_VideoManager->Get(name) : nullptr;
}

std::vector<std::string> ResourceManager::GetLoadedTextures() const
{
    return m_TextureManager ? m_TextureManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedModels() const
{
    return m_ModelManager ? m_ModelManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedShaders() const
{
    return m_ShaderManager ? m_ShaderManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedSounds() const
{
    return m_AudioManager ? m_AudioManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedSkyboxes() const
{
    return m_SkyboxManager ? m_SkyboxManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedAnimations() const
{
    return m_AnimationManager ? m_AnimationManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedVideos() const
{
    return m_VideoManager ? m_VideoManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedFonts() const
{
    return m_FontManager ? m_FontManager->GetAllNames() : std::vector<std::string>{};
}
std::vector<std::string> ResourceManager::GetLoadedFragments() const
{
    return m_FragmentManager ? m_FragmentManager->GetAllNames() : std::vector<std::string>{};
}

std::shared_ptr<Texture> ResourceManager::GetTextureAuto(const std::string& nameOrPath)
{
    if (nameOrPath.empty())
        return nullptr;
    auto t = GetTexture(nameOrPath);
    if (t)
        return t;

    // Check if any existing texture has this path
    std::string fullPath = FileSystem::getPath(nameOrPath);
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        for (const auto& def : m_ResourceDefinitions)
        {
            if (def.type == "Texture" && def.properties.count("Path"))
            {
                if (FileSystem::getPath(def.properties.at("Path")) == fullPath)
                {
                    return GetTexture(def.name);
                }
            }
        }
    }

    bool looksLikePath = nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath)
        return nullptr;
    if (!std::filesystem::exists(fullPath))
        return nullptr;

    LoadTexture(nameOrPath, nameOrPath);
    return GetTexture(nameOrPath);
}

std::shared_ptr<Model> ResourceManager::GetModelAuto(const std::string& nameOrPath, bool isStatic)
{
    if (nameOrPath.empty())
        return nullptr;
    auto m = GetModel(nameOrPath);
    if (m)
        return m;

    // Check if any existing model has this path
    std::string fullPath = FileSystem::getPath(nameOrPath);
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        for (const auto& def : m_ResourceDefinitions)
        {
            if (def.type == "Model" && def.properties.count("Path"))
            {
                if (FileSystem::getPath(def.properties.at("Path")) == fullPath)
                {
                    return GetModel(def.name);
                }
            }
        }
    }

    bool looksLikePath = nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath)
        return nullptr;
    if (!std::filesystem::exists(fullPath))
        return nullptr;
    LoadModel(nameOrPath, nameOrPath, isStatic);
    return GetModel(nameOrPath);
}

std::shared_ptr<Font> ResourceManager::GetFontAuto(const std::string& nameOrPath, unsigned int fontSize)
{
    if (nameOrPath.empty())
    {
        if (nameOrPath != "time")
            return GetFontAuto("time", fontSize);
        return nullptr;
    }

    auto f = GetFont(nameOrPath);
    if (f)
        return f;

    // Check if any existing font has this path and size
    std::string fullPath = FileSystem::getPath(nameOrPath);
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        for (const auto& def : m_ResourceDefinitions)
        {
            if (def.type == "Font")
            {
                if (FileSystem::getPath(def.properties.at("Path")) == fullPath &&
                    std::stoi(def.properties.at("Size")) == (int)fontSize)
                {
                    return GetFont(def.name);
                }
            }
        }
    }

    bool looksLikePath = nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath)
    {
        if (nameOrPath != "time")
            return GetFontAuto("time", fontSize);
        return nullptr;
    }

    if (!std::filesystem::exists(fullPath))
    {
        if (nameOrPath != "time")
            return GetFontAuto("time", fontSize);
        return nullptr;
    }

    LoadFont(nameOrPath, nameOrPath, fontSize);
    auto res = GetFont(nameOrPath);
    if (res)
        return res;

    if (nameOrPath != "time")
        return GetFontAuto("time", fontSize);
    return nullptr;
}

std::shared_ptr<IAudioSource> ResourceManager::GetSoundAuto(const std::string& nameOrPath, IAudioEngine* engine)
{
    if (nameOrPath.empty())
        return nullptr;
    auto s = GetSound(nameOrPath);
    if (s)
        return s;

    // Check if any existing sound has this path
    std::string fullPath = FileSystem::getPath(nameOrPath);
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        for (const auto& def : m_ResourceDefinitions)
        {
            if ((def.type == "Sound" || def.type == "Audio") && def.properties.count("Path"))
            {
                if (FileSystem::getPath(def.properties.at("Path")) == fullPath)
                {
                    return GetSound(def.name);
                }
            }
        }
    }

    bool looksLikePath = nameOrPath.find('/') != std::string::npos || nameOrPath.find('\\') != std::string::npos ||
                         nameOrPath.find('.') != std::string::npos;
    if (!looksLikePath)
        return nullptr;
    if (!std::filesystem::exists(fullPath))
        return nullptr;
    LoadSound(nameOrPath, nameOrPath, engine);
    return GetSound(nameOrPath);
}

std::shared_ptr<UIModel> ResourceManager::GetUIModel(const std::string& name)
{
    if (!m_UIModelManager)
        return nullptr;
    return m_UIModelManager->Get(name);
}

bool ResourceManager::HasUIModel(const std::string& name)
{
    if (!m_UIModelManager)
        return false;
    return m_UIModelManager->Has(name);
}

void ResourceManager::ClearResource()
{
    if (m_ShaderManager)
        m_ShaderManager->Clear();
    if (m_TextureManager)
        m_TextureManager->Clear();
    if (m_ModelManager)
        m_ModelManager->Clear();
    if (m_AudioManager)
        m_AudioManager->Clear();
    if (m_FontManager)
        m_FontManager->Clear();
    if (m_AnimationManager)
        m_AnimationManager->Clear();
    if (m_SkyboxManager)
        m_SkyboxManager->Clear();
    if (m_VideoManager)
        m_VideoManager->Clear();
    if (m_FragmentManager)
        m_FragmentManager->Clear();
    if (m_UIModelManager)
        m_UIModelManager->Clear();

    LOGGER_INFO("ResourceManager") << "All resources cleared via specialized managers";
}
