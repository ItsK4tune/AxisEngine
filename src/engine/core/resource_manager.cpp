#include <engine/core/resource_manager.h>
#include <engine/utils/filesystem.h>

ResourceManager::~ResourceManager()
{
    ClearResource();
}

void ResourceManager::LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath)
{
    shaders[name] = std::make_unique<Shader>(FileSystem::getPath(vsPath).c_str(), FileSystem::getPath(fsPath).c_str());
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
    models.clear();
    animations.clear();
    fonts.clear();
    uiModels.clear();
    sounds.clear();
    skyboxes.clear();

    std::cout << "[ResourceManager] Cleared all resources.\n";
}