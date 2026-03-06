#include <core/job_system.h>
#include <event/event_system.h>
#include <event/resource_events.h>
#include <iostream>
#include <resource/resource_manager.h>
#include <utils/filesystem.h>
#include <utils/logger.h>

ResourceManager::ResourceManager()
{
    m_ReloadListenerId = EventSystem::Instance().Subscribe<ResourceReloadEvent>([this](const ResourceReloadEvent &e)
                                                                                {
        if (e.type == "SHADER") {
            ReloadShader(e.name);
        } else if (e.type == "TEXTURE") {
            ReloadTexture(e.name);
        } });
}

void ResourceManager::Init(IShaderManager& shaderManager)
{
    m_ShaderManager = &shaderManager;
    m_ShaderCache = std::make_unique<ShaderCache>(*m_ShaderManager);
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
    FlushPendingModels();
    m_TextureCache.Update();
    m_ResourceWatcher.Update(dt);
}

void ResourceManager::UnloadTexture(const std::string &name)
{
    m_TextureCache.UnloadTexture(name);
}

void ResourceManager::UnloadModel(const std::string &name)
{
    m_ModelInstanceManager.UnloadModel(name);
}

void ResourceManager::ReloadShader(const std::string &name)
{
    LOGGER_INFO("HotReload") << "Reloading Shader: " << name;
    m_ShaderCache->Reload(name);
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
    m_ShaderCache->GetOrCompile(name, vShaderPath, fShaderPath);

    m_ResourceWatcher.Watch(name, vShaderPath, "SHADER", vShaderPath, fShaderPath, gShaderPath);
}

void ResourceManager::LoadTexture(const std::string &name, const std::string &path, bool async)
{
    LOGGER_INFO("ResourceManager") << "Loading texture: " << name << " from " << path << (async ? " (async)" : "");
    m_TextureCache.LoadTexture(name, FileSystem::getPath(path), async);

    m_ResourceWatcher.Watch(name, FileSystem::getPath(path), "TEXTURE");
}

void ResourceManager::LoadModel(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Loading model: " << name << " from " << fullPath;

    std::shared_ptr<Model> model = m_ModelInstanceManager.GetOrLoadModel(name, fullPath, isStatic);
    if (!model)
    {
        LOGGER_ERROR("ResourceManager") << "Failed to load model: " << path;
    }
    else
    {
        model->UploadToGPU();
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        m_ModelPaths[name] = {fullPath, isStatic};
        LOGGER_INFO("ResourceManager") << "Loaded model: " << name;
    }
}

void ResourceManager::LoadModelAsync(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_INFO("ResourceManager") << "Async loading model: " << name;

    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        m_ModelPaths[name] = {fullPath, isStatic};
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    JobSystem::Instance().Execute([this, promise, name, fullPath, isStatic]()
                                  {
        auto model = std::make_shared<Model>();
        model->LoadCPU(fullPath, isStatic);

        std::lock_guard<std::mutex> lock(m_PendingMutex);
        m_PendingModels.push_back({name, std::move(model)});
        promise->set_value(); });

    std::lock_guard<std::mutex> lock(m_PendingMutex);
    m_ActiveFutures.push_back(std::move(future));
}

void ResourceManager::LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName)
{
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_ModelPaths.find(modelName);
    if (it != m_ModelPaths.end())
    {
        std::shared_ptr<Model> model = m_ModelInstanceManager.GetOrLoadModel(modelName, it->second.path, it->second.isStatic);
        if (model)
        {
            LOGGER_INFO("ResourceManager") << "Loading animation: " << name << " for model " << modelName;
            m_AnimationCache.LoadAnimation(name, path, model.get());
        }
    }
    else
    {
        LOGGER_ERROR("ResourceManager") << "Model not found for animation: " << modelName;
    }
}

void ResourceManager::LoadFont(const std::string &name, const std::string &path, unsigned int fontSize)
{
    LOGGER_INFO("ResourceManager") << "Loading font: " << name << " (" << fontSize << "px) from " << path;
    m_FontCache.LoadFont(name, path, fontSize);
}

void ResourceManager::LoadSound(const std::string &name, const std::string &path, IAudioEngine *engine)
{
    LOGGER_INFO("ResourceManager") << "Loading sound: " << name;
    m_SoundCache.LoadSound(name, path, engine);
}

void ResourceManager::LoadSkybox(const std::string &name, const std::vector<std::string> &faces)
{
    auto skybox = std::make_shared<Skybox>();
    std::vector<std::string> fullFaces;
    for (const auto &face : faces)
    {
        fullFaces.push_back(FileSystem::getPath(face));
    }
    skybox->LoadCubemap(fullFaces);
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        m_Skyboxes[name] = skybox;
    }
    LOGGER_INFO("ResourceManager") << "Loaded skybox: " << name;
}

void ResourceManager::CreateUIModel(const std::string &name, UIType type)
{
    {
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
        m_UIModels[name] = std::make_shared<UIModel>(type);
    }
    LOGGER_INFO("ResourceManager") << "Created UI Model: " << name;
}

void ResourceManager::UnloadShader(const std::string &name)
{
    m_ShaderCache->Remove(name);
}

void ResourceManager::UnloadFont(const std::string &name)
{
    m_FontCache.Remove(name);
}

void ResourceManager::UnloadSound(const std::string &name)
{
    m_SoundCache.Remove(name);
}

void ResourceManager::UnloadSkybox(const std::string &name)
{
    m_Skyboxes.erase(name);
}

void ResourceManager::UnloadAnimation(const std::string &name)
{
    m_AnimationCache.Remove(name);
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string &name)
{
    return m_ShaderCache->GetShared(name);
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string &name)
{
    return m_TextureCache.GetTexture(name);
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string &name)
{
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    auto it = m_ModelPaths.find(name);
    if (it != m_ModelPaths.end())
    {
        auto model = m_ModelInstanceManager.GetOrLoadModel(name, it->second.path, it->second.isStatic);
        if (model)
            model->UploadToGPU();
        return model;
    }
    LOGGER_WARN("ResourceManager") << "Model path not found for: " << name;
    return nullptr;
}

std::shared_ptr<Animation> ResourceManager::GetAnimation(const std::string &name)
{
    return m_AnimationCache.GetAnimation(name);
}

std::shared_ptr<Font> ResourceManager::GetFont(const std::string &name)
{
    return m_FontCache.GetFont(name);
}

std::shared_ptr<IAudioSource> ResourceManager::GetSound(const std::string &name)
{
    return m_SoundCache.GetSound(name);
}

std::shared_ptr<Skybox> ResourceManager::GetSkybox(const std::string &name)
{
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    if (m_Skyboxes.find(name) != m_Skyboxes.end())
        return m_Skyboxes[name];

    LOGGER_WARN("ResourceManager") << "Skybox not found: " << name;
    return nullptr;
}

std::shared_ptr<UIModel> ResourceManager::GetUIModel(const std::string &name)
{
    std::lock_guard<std::mutex> lock(m_ResourceMutex);
    if (m_UIModels.find(name) != m_UIModels.end())
        return m_UIModels[name];

    LOGGER_WARN("ResourceManager") << "UI Model not found: " << name;
    return nullptr;
}

void ResourceManager::ClearResource()
{
    m_TextureCache.Clear();
    m_FontCache.Clear();
    m_SoundCache.Clear();
    m_AnimationCache.Clear();
    m_ModelInstanceManager.Clear();

    m_UIModels.clear();
    m_Skyboxes.clear();

    LOGGER_INFO("ResourceManager") << "All resources cleared";
}

void ResourceManager::FlushPendingModels()
{
    std::vector<PendingModel> readyModels;

    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);

        m_ActiveFutures.erase(
            std::remove_if(m_ActiveFutures.begin(), m_ActiveFutures.end(),
                           [](std::future<void> &f)
                           {
                               try {
                                   return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                               } catch (...) {
                                   return true;
                               }
                           }),
            m_ActiveFutures.end());

        for (auto it = m_PendingModels.begin(); it != m_PendingModels.end();)
        {
            if (it->model)
            {
                it->model->UploadToGPU();
                readyModels.push_back(std::move(*it));
                it = m_PendingModels.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto &pending : readyModels)
    {
        m_ModelInstanceManager.RegisterModel(pending.name, std::move(pending.model));
        LOGGER_INFO("ResourceManager") << "Async model registered: " << pending.name;
    }
}
