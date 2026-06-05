#include <resource/logic/texture_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <resource/type/resource_events.h>
#include <stb/stb_image.h>
#include <filesystem>

TextureManager::TextureManager(ITextureManager& lowLevelManager) : m_LowLevelManager(lowLevelManager)
{
}

void TextureManager::Initialize()
{
    // 1. Create a hardcoded "Emergency" Error Texture first
    unsigned char pinkBlack[] = {255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255};
    unsigned int errorID = m_LowLevelManager.GenTexture();
    m_LowLevelManager.BindTexture(TextureType::Texture2D, errorID);
    m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 2, 2, 0, TextureFormat::RGBA,
                                 DataType::UnsignedByte, pinkBlack);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

    m_ErrorTexture = std::make_shared<Texture>();
    m_ErrorTexture->id = errorID;
    m_ErrorTexture->width = 2;
    m_ErrorTexture->height = 2;
    m_ErrorTexture->nrComponents = 4;
    m_ErrorTexture->path = "internal://error_texture";
    m_Cache.Add("Internal_Error_Texture", m_ErrorTexture);

    // 2. Try to load the higher quality external error texture
    std::string errorTexturePath = "include/engine/asset/textures/error_checkerboard.tga";
    std::shared_ptr<Texture> externalError;
    if (std::filesystem::exists(FileSystem::getPath(errorTexturePath)))
        externalError = Load("External_Error_Texture", errorTexturePath, false, false);

    if (externalError && externalError->id != 0 && externalError->path != "internal://error_texture")
    {
        m_ErrorTexture = externalError;
        LOGGER_INFO("TextureManager") << "Initialized Externalized Error Texture (from file)";
    }
    else
    {
        LOGGER_WARN("TextureManager") << "Could not load external error texture, using hardcoded fallback.";
    }
}

TextureManager::~TextureManager()
{
    Clear();
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& name, const std::string& path, bool async,
                                              bool keepCpuData)
{
    if (auto existing = m_Cache.Get(name))
    {
        return existing;
    }

    std::string fullPath = FileSystem::getPath(path);

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        auto it = m_PathToTextureMap.find(fullPath);
        if (it != m_PathToTextureMap.end())
        {
            auto tex = it->second;
            m_Cache.Add(name, tex);
            m_NameToPathMap[name] = fullPath;
            m_NameKeepCpuDataMap[name] = keepCpuData;
            m_PathReferenceCounts[fullPath]++;
            LOGGER_INFO("TextureManager") << "Deduplicated texture load for path: " << path << " under name: " << name;
            return tex;
        }
    }

    if (async && m_AsyncEnabled)
    {
        auto promise = std::make_shared<std::promise<TextureData>>();
        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_AsyncLoads.push_back(promise->get_future());
        }

        auto tex = std::make_shared<Texture>();
        tex->id = 0;
        tex->path = path;
        m_Cache.Add(name, tex);

        {
            std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
            m_PathToTextureMap[fullPath] = tex;
            m_PathReferenceCounts[fullPath] = 1;
            m_NameToPathMap[name] = fullPath;
            m_NameKeepCpuDataMap[name] = keepCpuData;
        }

        JobSystem::Instance().Execute([promise, name, fullPath, keepCpuData]() {
            TextureData data;
            data.name = name;
            data.path = fullPath;
            data.keepCpuData = keepCpuData;
            stbi_set_flip_vertically_on_load(true);
            data.data = stbi_load(fullPath.c_str(), &data.width, &data.height, &data.nrComponents, 0);
            promise->set_value(data);
        });

        return tex;
    }
    else
    {
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            unsigned int textureID = m_LowLevelManager.GenTexture();

            TextureFormat format = TextureFormat::RGBA;
            InternalFormat iFormat = InternalFormat::RGBA8;

            if (nrComponents == 1)
            {
                format = TextureFormat::Red;
                iFormat = InternalFormat::R8;
            }
            else if (nrComponents == 3)
            {
                format = TextureFormat::RGB;
                iFormat = InternalFormat::RGB8;
            }
            else if (nrComponents == 4)
            {
                format = TextureFormat::RGBA;
                iFormat = InternalFormat::RGBA8;
            }

            m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
            m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format,
                                         DataType::UnsignedByte, data);
            m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);

            m_LowLevelManager.TexParameteri(
                TextureType::Texture2D, TextureParameter::WrapS,
                static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            m_LowLevelManager.TexParameteri(
                TextureType::Texture2D, TextureParameter::WrapT,
                static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                            static_cast<int>(TextureFilter::LinearMipmapLinear));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                            static_cast<int>(TextureFilter::Linear));

            if (m_MaxAnisotropy > 1.0f)
            {
                m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy,
                                                m_MaxAnisotropy);
            }

            auto tex = std::make_shared<Texture>();
            tex->id = textureID;
            tex->path = path;
            tex->width = width;
            tex->height = height;
            tex->nrComponents = nrComponents;
            tex->type = "texture_diffuse";

            if (keepCpuData)
                tex->pixelData = data;
            else
                stbi_image_free(data);

            m_Cache.Add(name, tex);

            {
                std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                m_PathToTextureMap[fullPath] = tex;
                m_PathReferenceCounts[fullPath] = 1;
                m_NameToPathMap[name] = fullPath;
                m_NameKeepCpuDataMap[name] = keepCpuData;
            }

            LOGGER_INFO("TextureManager") << "Loaded texture: " << name;
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
            return tex;
        }
        else
        {
            LOGGER_ERROR("TextureManager") << "Failed to load texture: " << path << "."
                                           << (m_StrictLoading ? " Strict loading is enabled." :
                                                                " Returning error texture.");
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            if (m_StrictLoading)
                return nullptr;
            return m_ErrorTexture;
        }
    }
}

std::shared_ptr<Texture> TextureManager::Get(const std::string& name)
{
    return m_Cache.Get(name);
}

void TextureManager::Unload(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
    if (auto tex = m_Cache.Get(name))
    {
        auto pathIt = m_NameToPathMap.find(name);
        if (pathIt != m_NameToPathMap.end())
        {
            std::string fullPath = pathIt->second;
            m_PathReferenceCounts[fullPath]--;
            if (m_PathReferenceCounts[fullPath] <= 0)
            {
                if (tex->id != 0)
                {
                    m_LowLevelManager.DeleteTextures(1, &tex->id);
                }
                m_PathToTextureMap.erase(fullPath);
                m_PathReferenceCounts.erase(fullPath);
                LOGGER_INFO("TextureManager") << "Fully unloaded physical texture from GPU: " << fullPath;
            }
            m_NameToPathMap.erase(pathIt);
            m_NameKeepCpuDataMap.erase(name);
        }
        else
        {
            if (tex->id != 0)
            {
                m_LowLevelManager.DeleteTextures(1, &tex->id);
            }
        }
        m_Cache.Remove(name);
        LOGGER_INFO("TextureManager") << "Removed texture name alias from cache: " << name;
    }
}

bool TextureManager::Reload(const std::string& name)
{
    std::string fullPath;
    bool keepCpuData = false;
    std::shared_ptr<Texture> texture;

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        auto pathIt = m_NameToPathMap.find(name);
        if (pathIt == m_NameToPathMap.end())
        {
            LOGGER_ERROR("TextureManager") << "Cannot reload texture without a file path: " << name;
            return false;
        }

        fullPath = pathIt->second;
        auto keepIt = m_NameKeepCpuDataMap.find(name);
        keepCpuData = keepIt != m_NameKeepCpuDataMap.end() ? keepIt->second : false;
    }

    texture = m_Cache.Get(name);
    if (!texture)
    {
        LOGGER_ERROR("TextureManager") << "Cannot reload missing texture: " << name;
        return false;
    }

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
    if (!data)
    {
        LOGGER_ERROR("TextureManager") << "Failed to reload texture: " << name << " from " << fullPath;
        EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
        return false;
    }

    TextureFormat format = TextureFormat::RGBA;
    InternalFormat iFormat = InternalFormat::RGBA8;
    if (nrComponents == 1)
    {
        format = TextureFormat::Red;
        iFormat = InternalFormat::R8;
    }
    else if (nrComponents == 3)
    {
        format = TextureFormat::RGB;
        iFormat = InternalFormat::RGB8;
    }

    const unsigned int oldId = texture->id;
    unsigned int textureID = m_LowLevelManager.GenTexture();
    m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
    m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format,
                                 DataType::UnsignedByte, data);
    m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);
    m_LowLevelManager.TexParameteri(
        TextureType::Texture2D, TextureParameter::WrapS,
        static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
    m_LowLevelManager.TexParameteri(
        TextureType::Texture2D, TextureParameter::WrapT,
        static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                    static_cast<int>(TextureFilter::LinearMipmapLinear));
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                    static_cast<int>(TextureFilter::Linear));

    if (m_MaxAnisotropy > 1.0f)
    {
        m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy,
                                        m_MaxAnisotropy);
    }

    if (oldId != 0 && (!m_ErrorTexture || oldId != m_ErrorTexture->id))
    {
        m_LowLevelManager.DeleteTextures(1, &oldId);
    }

    texture->id = textureID;
    texture->path = fullPath;
    texture->width = width;
    texture->height = height;
    texture->nrComponents = nrComponents;

    if (keepCpuData)
        texture->pixelData = data;
    else
        stbi_image_free(data);

    LOGGER_INFO("TextureManager") << "Reloaded texture: " << name;
    EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
    return true;
}

void TextureManager::Update(float dt)
{
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    auto it = m_AsyncLoads.begin();
    while (it != m_AsyncLoads.end())
    {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            TextureData data = it->get();
            if (data.data)
            {
                unsigned int textureID = m_LowLevelManager.GenTexture();

                TextureFormat format = TextureFormat::RGBA;
                InternalFormat iFormat = InternalFormat::RGBA8;
                if (data.nrComponents == 1)
                {
                    format = TextureFormat::Red;
                    iFormat = InternalFormat::R8;
                }
                else if (data.nrComponents == 3)
                {
                    format = TextureFormat::RGB;
                    iFormat = InternalFormat::RGB8;
                }
                else if (data.nrComponents == 4)
                {
                    format = TextureFormat::RGBA;
                    iFormat = InternalFormat::RGBA8;
                }

                m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
                m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, data.width, data.height, 0, format,
                                             DataType::UnsignedByte, data.data);
                m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);
                m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                                static_cast<int>(TextureFilter::LinearMipmapLinear));
                m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                                static_cast<int>(TextureFilter::Linear));

                if (m_MaxAnisotropy > 1.0f)
                {
                    m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy,
                                                    m_MaxAnisotropy);
                }

                if (auto tex = m_Cache.Get(data.name))
                {
                    tex->id = textureID;
                    tex->width = data.width;
                    tex->height = data.height;
                    tex->nrComponents = data.nrComponents;
                    if (data.keepCpuData)
                        tex->pixelData = data.data;
                    else
                        stbi_image_free(data.data);
                }
                LOGGER_INFO("TextureManager") << "Async texture loaded: " << data.name;
                EventManager::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", true});
            }
            else
            {
                if (m_StrictLoading)
                {
                    {
                        std::lock_guard<std::mutex> pathLock(m_DeduplicationMutex);
                        auto pathIt = m_NameToPathMap.find(data.name);
                        if (pathIt != m_NameToPathMap.end())
                        {
                            const std::string fullPath = pathIt->second;
                            m_PathToTextureMap.erase(fullPath);
                            m_PathReferenceCounts.erase(fullPath);
                            m_NameToPathMap.erase(pathIt);
                            m_NameKeepCpuDataMap.erase(data.name);
                        }
                    }
                    m_Cache.Remove(data.name);
                    LOGGER_ERROR("TextureManager") << "Failed async texture: " << data.name
                                                   << ". Strict loading is enabled.";
                }
                else
                {
                    if (auto tex = m_Cache.Get(data.name))
                    {
                        if (m_ErrorTexture)
                        {
                            tex->id = m_ErrorTexture->id;
                            tex->width = m_ErrorTexture->width;
                            tex->height = m_ErrorTexture->height;
                            tex->nrComponents = m_ErrorTexture->nrComponents;
                        }
                    }
                    LOGGER_ERROR("TextureManager")
                        << "Failed async texture: " << data.name << ". Falling back to error texture.";
                }
                EventManager::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", false});
            }
            it = m_AsyncLoads.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TextureManager::Clear()
{
    auto names = m_Cache.GetAllNames();
    for (const auto& name : names)
    {
        Unload(name);
    }
    m_Cache.Clear();

    {
        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
        m_PathToTextureMap.clear();
        m_PathReferenceCounts.clear();
        m_NameToPathMap.clear();
        m_NameKeepCpuDataMap.clear();
    }

    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    m_AsyncLoads.clear();
}

std::shared_ptr<Texture> TextureManager::CreateFromData(const std::string& name, const unsigned char* pixels, int width, int height, int nrComponents, bool keepCpuData)
{
    if (auto existing = m_Cache.Get(name))
    {
        return existing;
    }

    auto tex = std::make_shared<Texture>();
    tex->width = width;
    tex->height = height;
    tex->nrComponents = nrComponents;
    tex->path = "memory://" + name;

    unsigned int textureID = m_LowLevelManager.GenTexture();
    tex->id = textureID;

    InternalFormat iFormat = InternalFormat::RGB8;
    TextureFormat format = TextureFormat::RGB;
    if (nrComponents == 1)
    {
        iFormat = InternalFormat::R8;
        format = TextureFormat::Red;
    }
    else if (nrComponents == 3)
    {
        iFormat = InternalFormat::RGB8;
        format = TextureFormat::RGB;
    }
    else if (nrComponents == 4)
    {
        iFormat = InternalFormat::RGBA8;
        format = TextureFormat::RGBA;
    }

    m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
    m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format,
                                 DataType::UnsignedByte, pixels);
    m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);

    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, (int)TextureWrap::Repeat);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, (int)TextureWrap::Repeat);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::LinearMipmapLinear);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Linear);

    if (m_MaxAnisotropy > 1.0f)
    {
        m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy, m_MaxAnisotropy);
    }

    if (keepCpuData && pixels)
    {
        tex->pixelData = new unsigned char[width * height * nrComponents];
        std::memcpy(tex->pixelData, pixels, width * height * nrComponents);
    }

    m_Cache.Add(name, tex);
    return tex;
}
