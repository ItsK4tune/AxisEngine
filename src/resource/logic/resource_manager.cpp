#include <resource/logic/resource_manager.h>
#include <audio/interface/i_audio_source.h>
#include <core/logic/axis_assert.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/loader_utils.h>
#include <core/logic/loader_strategies.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <core/type/app_config.h>
#include <resource/type/resource_events.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

ResourceManager::ResourceManager()
{
    SubscribeReloadEvents();
}

void ResourceManager::SubscribeReloadEvents()
{
    m_ReloadListenerId = EventManager::Instance().Subscribe<ResourceReloadEvent>([this](const ResourceReloadEvent& e) {
        if (e.type == "SHADER")
        {
            ReloadShader(e.name);
        }
        else if (e.type == "COMPUTE_SHADER")
        {
            ReloadComputeShader(e.name);
        }
        else if (e.type == "Texture")
        {
            ReloadTexture(e.name);
        }
    });
}

void ResourceManager::RegisterLoader(std::unique_ptr<ILoaderStrategy> strategy)
{
    RegisterLoaderInternal(std::move(strategy), true);
}

bool ResourceManager::RegisterLoaderInternal(std::unique_ptr<ILoaderStrategy> strategy, bool replaceExisting)
{
    if (!strategy)
    {
        LOGGER_WARN("ResourceManager") << "Ignoring null loading strategy";
        return false;
    }
    std::string name = strategy->GetName();
    if (name.empty())
    {
        LOGGER_WARN("ResourceManager") << "Ignoring loading strategy with an empty name";
        return false;
    }
    {
        std::unique_lock lock(m_StrategyMutex);
        if (!replaceExisting && m_Strategies.contains(name))
            return false;
        m_Strategies[name] = std::shared_ptr<ILoaderStrategy>(std::move(strategy));
    }
    LOGGER_INFO("ResourceManager") << "Registered loading strategy: " << name;
    return true;
}

bool ResourceManager::LoadUnified(const std::string& type, const std::string& path)
{
    std::shared_ptr<ILoaderStrategy> strategy;
    {
        std::shared_lock lock(m_StrategyMutex);
        const auto it = m_Strategies.find(type);
        if (it != m_Strategies.end())
            strategy = it->second;
    }
    if (strategy)
    {
        LOGGER_INFO("ResourceManager") << "Dispatching '" << path << "' to strategy: " << type;
        return strategy->Load(path);
    }

    LOGGER_ERROR("ResourceManager") << "No strategy found for type: " << type;
    return false;
}

std::vector<std::string> ResourceManager::GetRegisteredLoaderTypes() const
{
    std::shared_lock lock(m_StrategyMutex);
    std::vector<std::string> types;
    for (const auto& pair : m_Strategies)
    {
        types.push_back(pair.first);
    }
    std::sort(types.begin(), types.end());
    return types;
}

void ResourceManager::Initialize(IShaderManager* shaderManager, ITextureManager* textureManager,
                                 IAudioEngine* audioEngine)
{
    m_IsShutdown = false;
    if (m_ReloadListenerId == -1)
        SubscribeReloadEvents();
    m_HeadlessMode = (shaderManager == nullptr && textureManager == nullptr);
    m_LowLevelShaderManager = shaderManager;

    if (shaderManager)
        m_ShaderManager = std::make_unique<ShaderManager>(*shaderManager);
    if (textureManager)
        m_TextureManager = std::make_unique<TextureManager>(*textureManager);

    m_ModelManager = std::make_unique<ModelManager>();
    if (audioEngine)
        m_AudioManager = std::make_unique<AudioAssetManager>(*audioEngine);
    m_FontManager = std::make_unique<FontManager>();
    m_SkyboxManager = std::make_unique<SkyboxManager>();
    m_AnimationManager = std::make_unique<AnimationManager>(*m_ModelManager);
    m_VideoManager = std::make_unique<VideoManager>();
    m_FragmentManager = std::make_unique<FragmentAssetManager>();
    m_UIModelManager = std::make_unique<UIModelManager>();
    RegisterDefaultLoaderStrategies(*this);

    if (m_ShaderManager)
        m_ShaderManager->Initialize();
    if (m_TextureManager)
        m_TextureManager->Initialize();
    // ModelManager::Initialize requires cubeModel from load.axs, so it should be called after LoadScene
}

void ResourceManager::InitializeHeadless()
{
    m_IsShutdown = false;
    if (m_ReloadListenerId == -1)
        SubscribeReloadEvents();
    m_HeadlessMode = true;
    m_ModelManager = std::make_unique<ModelManager>();
    m_FragmentManager = std::make_unique<FragmentAssetManager>();
    m_AnimationManager = std::make_unique<AnimationManager>(*m_ModelManager);
    RegisterDefaultLoaderStrategies(*this);
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
    Shutdown();
}

void ResourceManager::Shutdown()
{
    if (m_IsShutdown)
        return;

    m_ResourceWatcher.SetEnabled(false);

    if (m_ReloadListenerId != -1)
    {
        EventManager::Instance().Unsubscribe<ResourceReloadEvent>(m_ReloadListenerId);
        m_ReloadListenerId = -1;
    }
    ClearResource();
    m_LowLevelShaderManager = nullptr;
    m_IsShutdown = true;
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

void ResourceManager::SetStrictAssetLoading(bool strict)
{
    m_StrictAssetLoading = strict;
    if (m_ShaderManager)
        m_ShaderManager->SetStrictLoading(strict);
    if (m_TextureManager)
        m_TextureManager->SetStrictLoading(strict);
    if (m_ModelManager)
        m_ModelManager->SetStrictLoading(strict);
}

void ResourceManager::SetAsyncUploadBudget(bool enabled, size_t maxModelUploadsPerFrame,
                                           size_t maxTextureUploadsPerFrame)
{
    if (m_ModelManager)
        m_ModelManager->SetCompletedLoadBudget(enabled, maxModelUploadsPerFrame);
    if (m_TextureManager)
        m_TextureManager->SetCompletedLoadBudget(enabled, maxTextureUploadsPerFrame);
}

void ResourceManager::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    m_ResourceWatcher.SetEnabled(config.resourceHotReloadEnabled);
    SetAsyncUploadBudget(config.resourceUploadBudgetEnabled,
                         static_cast<size_t>(config.maxModelUploadsPerFrame),
                         static_cast<size_t>(config.maxTextureUploadsPerFrame));
    if (m_ModelManager)
        m_ModelManager->SetDiscardCpuMeshDataAfterUpload(config.discardCpuMeshDataAfterUpload);
    if (m_TextureManager)
        m_TextureManager->SetCompressedTextureLoadingEnabled(config.compressedTextureLoadingEnabled);
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

void ResourceManager::ReloadComputeShader(const std::string& name)
{
    const auto found = m_ComputeShaders.find(name);
    if (found != m_ComputeShaders.end() && found->second)
    {
        LOGGER_INFO("HotReload") << "Reloading compute shader: " << name;
        found->second->Reload();
    }
}

void ResourceManager::ReloadTexture(const std::string& name)
{
    LOGGER_INFO("HotReload") << "Reloading Texture: " << name;
    if (m_TextureManager)
    {
        m_TextureManager->Reload(name);
    }
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

std::vector<ResourceManager::ResourceDefinition> ResourceManager::GetResourceDefinitions() const
{
    std::lock_guard lock(m_ResourceMutex);
    return m_ResourceDefinitions;
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
                    fsContent.find("gPBRParams") != std::string::npos)
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

bool ResourceManager::LoadComputeShader(const std::string& name, const std::string& path)
{
    if (!m_LowLevelShaderManager || name.empty() || path.empty())
        return false;
    const std::string fullPath = FileSystem::getPath(path);
    auto shader = std::make_shared<ComputeShader>(*m_LowLevelShaderManager, fullPath);
    if (!shader->IsValid())
        return false;
    m_ComputeShaders[name] = std::move(shader);
    m_ResourceWatcher.Watch(name, fullPath, "COMPUTE_SHADER");
    AddResourceDefinition("ComputeShader", name, {{"Path", path}});
    return true;
}

void ResourceManager::UnloadComputeShader(const std::string& name)
{
    m_ComputeShaders.erase(name);
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

void ResourceManager::CreateTextureFromData(const std::string& name, const unsigned char* pixels, int width, int height,
                                            int nrComponents, bool keepCpuData)
{
    if (m_HeadlessMode || !m_TextureManager)
        return;
    m_TextureManager->CreateFromData(name, pixels, width, height, nrComponents, keepCpuData);
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

void ResourceManager::LoadSound(const std::string& name, const std::string& path)
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

bool ResourceManager::LoadVideo(const std::string& name, const std::string& path)
{
    if (m_HeadlessMode || !m_VideoManager || name.empty() || path.empty())
        return false;
    if (!m_VideoManager->Load(name, FileSystem::getPath(path)))
        return false;
    AddResourceDefinition("Video", name, {{"Path", path}});
    return true;
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
void ResourceManager::UnloadVideo(const std::string& name)
{
    if (m_VideoManager)
        m_VideoManager->Unload(name);
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name)
{
    return m_ShaderManager ? m_ShaderManager->Get(name) : nullptr;
}

std::shared_ptr<ComputeShader> ResourceManager::GetComputeShader(const std::string& name)
{
    const auto found = m_ComputeShaders.find(name);
    return found != m_ComputeShaders.end() ? found->second : nullptr;
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
std::vector<std::string> ResourceManager::GetLoadedComputeShaders() const
{
    std::vector<std::string> names;
    names.reserve(m_ComputeShaders.size());
    for (const auto& [name, shader] : m_ComputeShaders)
        names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
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
    std::string matchingFont;
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        for (const auto& def : m_ResourceDefinitions)
        {
            const auto path = def.properties.find("Path");
            const auto size = def.properties.find("Size");
            if (def.type == "Font" && path != def.properties.end() && size != def.properties.end() &&
                FileSystem::getPath(path->second) == fullPath &&
                LoaderUtils::SafeStoi(size->second, -1) == static_cast<int>(fontSize))
            {
                matchingFont = def.name;
                break;
            }
        }
    }
    if (!matchingFont.empty())
        return GetFont(matchingFont);

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

std::shared_ptr<IAudioSource> ResourceManager::GetSoundAuto(const std::string& nameOrPath)
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
    LoadSound(nameOrPath, nameOrPath);
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
    m_ComputeShaders.clear();
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
