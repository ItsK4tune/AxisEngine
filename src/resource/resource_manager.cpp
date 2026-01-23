#include <resource/resource_manager.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <iostream>

ResourceManager::~ResourceManager()
{
    ClearResource();
}

void ResourceManager::Update()
{
    m_TextureCache.Update();
    m_ResourceWatcher.Update(0.016f);
}

void ResourceManager::ReloadShader(const std::string &name)
{
    LOGGER_INFO("HotReload") << "Reloading Shader: " << name;
    m_ShaderCache.Reload(name);
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

    m_ShaderCache.GetOrCompile(name, vShaderPath, fShaderPath);

    m_ResourceWatcher.SetShaderReloadCallback([this](const std::string &n)
                                              { ReloadShader(n); });
    m_ResourceWatcher.Watch(name, vShaderPath, "SHADER", vShaderPath, fShaderPath, gShaderPath);
}

void ResourceManager::LoadTexture(const std::string &name, const std::string &path, bool async)
{
    m_TextureCache.LoadTexture(name, FileSystem::getPath(path), async);

    m_ResourceWatcher.SetTextureReloadCallback([this](const std::string &n)
                                               { ReloadTexture(n); });
    m_ResourceWatcher.Watch(name, FileSystem::getPath(path), "TEXTURE");
}

void ResourceManager::LoadModel(const std::string &name, const std::string &path, bool isStatic)
{
    std::string fullPath = FileSystem::getPath(path);
    LOGGER_DEBUG("ResourceManager") << "Loading model: " << name << " from " << fullPath;

    Model *model = m_ModelInstanceManager.GetOrLoadModel(name, fullPath, isStatic);
    if (!model)
    {
        LOGGER_ERROR("ResourceManager") << "Failed to load model: " << path;
    }
    else
    {
        m_ModelPaths[name] = {name, isStatic};
        LOGGER_INFO("ResourceManager") << "Loaded model: " << name;
    }
}

void ResourceManager::LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName)
{
    auto it = m_ModelPaths.find(modelName);
    if (it != m_ModelPaths.end())
    {
        Model *model = m_ModelInstanceManager.GetOrLoadModel(modelName, it->second.path, it->second.isStatic);
        if (model)
        {
            LOGGER_DEBUG("ResourceManager") << "Loading animation: " << name << " for model " << modelName;
            m_AnimationCache.LoadAnimation(name, path, model);
        }
    }
    else
    {
        LOGGER_ERROR("ResourceManager") << "Model not found for animation: " << modelName;
    }
}

void ResourceManager::LoadFont(const std::string &name, const std::string &path, unsigned int fontSize)
{
    m_FontCache.LoadFont(name, path, fontSize);
}

void ResourceManager::LoadSound(const std::string &name, const std::string &path, irrklang::ISoundEngine *engine)
{
    LOGGER_DEBUG("ResourceManager") << "Loading sound: " << name;
    m_SoundCache.LoadSound(name, path, engine);
}

void ResourceManager::LoadSkybox(const std::string &name, const std::vector<std::string> &faces)
{
    auto skybox = std::make_unique<Skybox>();
    skybox->LoadCubemap(faces);
    m_Skyboxes[name] = std::move(skybox);
    LOGGER_INFO("ResourceManager") << "Loaded skybox: " << name;
}

void ResourceManager::CreateUIModel(const std::string &name, UIType type)
{
    m_UIModels[name] = std::make_unique<UIModel>(type);
    LOGGER_INFO("ResourceManager") << "Created UI Model: " << name;
}

Shader *ResourceManager::GetShader(const std::string &name)
{
    return m_ShaderCache.Get(name);
}

Texture *ResourceManager::GetTexture(const std::string &name)
{
    return m_TextureCache.GetTexture(name);
}

Model *ResourceManager::GetModel(const std::string &name)
{
    auto it = m_ModelPaths.find(name);
    if (it != m_ModelPaths.end())
    {
        return m_ModelInstanceManager.GetOrLoadModel(name, it->second.path, it->second.isStatic);
    }
    LOGGER_WARN("ResourceManager") << "Model path not found for: " << name;
    return nullptr;
}

Animation *ResourceManager::GetAnimation(const std::string &name)
{
    return m_AnimationCache.GetAnimation(name);
}

Font *ResourceManager::GetFont(const std::string &name)
{
    return m_FontCache.GetFont(name);
}

irrklang::ISoundSource *ResourceManager::GetSound(const std::string &name)
{
    return m_SoundCache.GetSound(name);
}

Skybox *ResourceManager::GetSkybox(const std::string &name)
{
    if (m_Skyboxes.find(name) != m_Skyboxes.end())
        return m_Skyboxes[name].get();

    LOGGER_WARN("ResourceManager") << "Skybox not found: " << name;
    return nullptr;
}

UIModel *ResourceManager::GetUIModel(const std::string &name)
{
    if (m_UIModels.find(name) != m_UIModels.end())
        return m_UIModels[name].get();

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