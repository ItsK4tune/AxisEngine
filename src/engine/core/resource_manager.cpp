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

#include <stb_image.h>

void ResourceManager::LoadTexture(const std::string &name, const std::string &path)
{
    std::string fullPath = FileSystem::getPath(path);
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 4);
    if (data)
    {
        GLenum format = GL_RGBA;

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