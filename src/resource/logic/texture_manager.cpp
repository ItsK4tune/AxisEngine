#include <resource/logic/texture_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <resource/logic/stb_image_loader.h>
#include <resource/type/resource_events.h>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
bool IsDdsPath(const std::string& path)
{
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".dds";
}

void ConfigureTextureSampling(ITextureManager& manager, float anisotropy, bool clampAlpha)
{
    manager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS,
                          static_cast<int>(clampAlpha ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
    manager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT,
                          static_cast<int>(clampAlpha ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
    manager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                          static_cast<int>(TextureFilter::LinearMipmapLinear));
    manager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                          static_cast<int>(TextureFilter::Linear));
    if (anisotropy > 1.0f)
        manager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy, anisotropy);
}

unsigned int UploadCompressedTexture(ITextureManager& manager, const CompressedTextureData& data, float anisotropy)
{
    if (data.mips.empty())
        return 0;
    const unsigned int textureId = manager.GenTexture();
    manager.BindTexture(TextureType::Texture2D, textureId);
    for (size_t level = 0; level < data.mips.size(); ++level)
    {
        const auto& mip = data.mips[level];
        if (!manager.CompressedTexImage2D(TextureType::Texture2D, static_cast<int>(level), data.format, mip.width,
                                          mip.height, mip.bytes.size(), mip.bytes.data()))
        {
            manager.DeleteTexture(textureId);
            return 0;
        }
    }
    if (data.mips.size() == 1)
        manager.GenerateMipmap(TextureType::Texture2D);
    ConfigureTextureSampling(manager, anisotropy, data.components == 4);
    return textureId;
}
}  // namespace

void TextureManager::StbiPixelDeleter::operator()(unsigned char* pixels) const
{
    if (pixels)
        StbImageLoader::Free(pixels);
}

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
    std::string errorTexturePath = "asset://textures/error_checkerboard.tga";
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
        const bool compressedTextureLoadingEnabled = m_CompressedTextureLoadingEnabled;
        auto promise = std::make_shared<std::promise<TextureData>>();
        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            generation = ++m_LoadGenerations[fullPath];
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

        JobSystem::Instance().Execute(
            [promise, name, fullPath, keepCpuData, generation, compressedTextureLoadingEnabled]() {
            TextureData data;
            data.name = name;
            data.path = fullPath;
            data.keepCpuData = keepCpuData;
            data.generation = generation;
            try
            {
                if (IsDdsPath(fullPath) && compressedTextureLoadingEnabled)
                {
                    data.compressed = std::make_shared<CompressedTextureData>();
                    std::string error;
                    if (!LoadDdsTexture(fullPath, *data.compressed, &error))
                    {
                        LOGGER_ERROR("TextureManager") << "DDS decode failed for " << fullPath << ": " << error;
                        data.compressed.reset();
                    }
                    else
                    {
                        data.width = data.compressed->width;
                        data.height = data.compressed->height;
                        data.nrComponents = data.compressed->components;
                    }
                }
                else if (!IsDdsPath(fullPath))
                {
                    data.data.reset(StbImageLoader::Load(fullPath.c_str(), &data.width, &data.height,
                                                         &data.nrComponents, 0, true));
                }
            }
            catch (...)
            {
                LOGGER_ERROR("TextureManager") << "Async texture worker failed for: " << fullPath;
            }
            promise->set_value(std::move(data));
            });

        return tex;
    }
    else
    {
        if (IsDdsPath(fullPath))
        {
            if (!m_CompressedTextureLoadingEnabled)
            {
                LOGGER_WARN("TextureManager") << "Compressed DDS loading is disabled: " << path;
                EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
                return m_StrictLoading ? nullptr : m_ErrorTexture;
            }
            CompressedTextureData compressed;
            std::string error;
            if (LoadDdsTexture(fullPath, compressed, &error))
            {
                const unsigned int textureID = UploadCompressedTexture(m_LowLevelManager, compressed, m_MaxAnisotropy);
                if (textureID != 0)
                {
                    auto tex = std::make_shared<Texture>();
                    tex->id = textureID;
                    tex->path = path;
                    tex->width = compressed.width;
                    tex->height = compressed.height;
                    tex->nrComponents = compressed.components;
                    tex->type = "texture_diffuse";
                    m_Cache.Add(name, tex);
                    {
                        std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
                        m_PathToTextureMap[fullPath] = tex;
                        m_PathReferenceCounts[fullPath] = 1;
                        m_NameToPathMap[name] = fullPath;
                        m_NameKeepCpuDataMap[name] = false;
                    }
                    if (keepCpuData)
                        LOGGER_WARN("TextureManager") << "keepCpuData is not available for compressed DDS: " << path;
                    LOGGER_INFO("TextureManager") << "Loaded compressed DDS texture: " << name;
                    EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
                    return tex;
                }
                error = "graphics backend rejected the BCn format";
            }
            LOGGER_ERROR("TextureManager") << "Failed to load DDS texture " << path << ": " << error;
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            return m_StrictLoading ? nullptr : m_ErrorTexture;
        }

        int width, height, nrComponents;
        unsigned char* data = StbImageLoader::Load(fullPath.c_str(), &width, &height, &nrComponents, 0, true);

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
                tex->SetPixelData(data, Texture::PixelDataOwnership::Stbi);
            else
                StbImageLoader::Free(data);

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
            LOGGER_ERROR("TextureManager")
                << "Failed to load texture: " << path << "."
                << (m_StrictLoading ? " Strict loading is enabled." : " Returning error texture.");
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
    std::string fullyUnloadedPath;
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
                    if (tex->id != 0 && (!m_ErrorTexture || tex->id != m_ErrorTexture->id))
                    {
                        m_LowLevelManager.DeleteTextures(1, &tex->id);
                    }
                    tex->ReleasePixelData();
                    m_PathToTextureMap.erase(fullPath);
                    m_PathReferenceCounts.erase(fullPath);
                    fullyUnloadedPath = fullPath;
                    LOGGER_INFO("TextureManager") << "Fully unloaded physical texture from GPU: " << fullPath;
                }
                m_NameToPathMap.erase(pathIt);
                m_NameKeepCpuDataMap.erase(name);
            }
            else
            {
                if (tex->id != 0 && (!m_ErrorTexture || tex->id != m_ErrorTexture->id))
                {
                    m_LowLevelManager.DeleteTextures(1, &tex->id);
                }
                tex->ReleasePixelData();
            }
            m_Cache.Remove(name);
            LOGGER_INFO("TextureManager") << "Removed texture name alias from cache: " << name;
        }
    }
    if (!fullyUnloadedPath.empty())
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        ++m_LoadGenerations[fullyUnloadedPath];
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

    if (IsDdsPath(fullPath))
    {
        if (!m_CompressedTextureLoadingEnabled)
        {
            LOGGER_WARN("TextureManager") << "Compressed DDS reload is disabled: " << name;
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            return false;
        }
        CompressedTextureData compressed;
        std::string error;
        if (!LoadDdsTexture(fullPath, compressed, &error))
        {
            LOGGER_ERROR("TextureManager") << "Failed to reload DDS " << name << ": " << error;
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            return false;
        }
        const unsigned int newId = UploadCompressedTexture(m_LowLevelManager, compressed, m_MaxAnisotropy);
        if (newId == 0)
        {
            LOGGER_ERROR("TextureManager") << "Graphics backend rejected DDS format while reloading " << name;
            EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            return false;
        }
        const unsigned int oldId = texture->id;
        texture->id = newId;
        texture->path = fullPath;
        texture->width = compressed.width;
        texture->height = compressed.height;
        texture->nrComponents = compressed.components;
        texture->ReleasePixelData();
        if (oldId != 0 && (!m_ErrorTexture || oldId != m_ErrorTexture->id))
            m_LowLevelManager.DeleteTexture(oldId);
        if (keepCpuData)
            LOGGER_WARN("TextureManager") << "keepCpuData is not available for compressed DDS: " << fullPath;
        EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
        return true;
    }

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    unsigned char* data = StbImageLoader::Load(fullPath.c_str(), &width, &height, &nrComponents, 0, true);
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
    m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format, DataType::UnsignedByte,
                                 data);
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
        texture->SetPixelData(data, Texture::PixelDataOwnership::Stbi);
    else
    {
        texture->ReleasePixelData();
        StbImageLoader::Free(data);
    }

    LOGGER_INFO("TextureManager") << "Reloaded texture: " << name;
    EventManager::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
    return true;
}

void TextureManager::Update(float dt)
{
    std::vector<TextureData> completed;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        const size_t completionLimit =
            m_CompletedLoadBudgetEnabled ? m_MaxCompletedLoadsPerFrame : m_AsyncLoads.size();
        completed.reserve((std::min)(completionLimit, m_AsyncLoads.size()));
        auto it = m_AsyncLoads.begin();
        while (it != m_AsyncLoads.end() && completed.size() < completionLimit)
        {
            if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                completed.push_back(it->get());
                it = m_AsyncLoads.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto& data : completed)
    {
        bool currentGeneration = false;
        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            const auto generation = m_LoadGenerations.find(data.path);
            currentGeneration = generation != m_LoadGenerations.end() && generation->second == data.generation;
        }
        if (!currentGeneration)
            continue;

        std::shared_ptr<Texture> tex;
        std::vector<std::string> aliases;
        bool keepCpuData = data.keepCpuData;
        {
            std::lock_guard<std::mutex> lock(m_DeduplicationMutex);
            if (const auto pathTexture = m_PathToTextureMap.find(data.path); pathTexture != m_PathToTextureMap.end())
                tex = pathTexture->second;
            for (const auto& [alias, path] : m_NameToPathMap)
            {
                if (path != data.path)
                    continue;
                aliases.push_back(alias);
                if (const auto keep = m_NameKeepCpuDataMap.find(alias); keep != m_NameKeepCpuDataMap.end())
                    keepCpuData = keepCpuData || keep->second;
            }
        }
        if (data.compressed && tex)
        {
            const unsigned int textureID =
                UploadCompressedTexture(m_LowLevelManager, *data.compressed, m_MaxAnisotropy);
            if (textureID != 0)
            {
                tex->id = textureID;
                tex->width = data.compressed->width;
                tex->height = data.compressed->height;
                tex->nrComponents = data.compressed->components;
                if (keepCpuData)
                    LOGGER_WARN("TextureManager") << "keepCpuData is not available for compressed DDS: " << data.path;
                LOGGER_INFO("TextureManager") << "Async compressed DDS loaded: " << data.name;
                for (const auto& alias : aliases)
                    EventManager::Instance().Publish(ResourceLoadedEvent{alias, "Texture", true});
                continue;
            }
            data.compressed.reset();
        }
        if (data.data && tex)
        {
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

            const unsigned int textureID = m_LowLevelManager.GenTexture();
            m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
            m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, data.width, data.height, 0, format,
                                         DataType::UnsignedByte, data.data.get());
            m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);
            const int wrap = static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge
                                                                            : TextureWrap::Repeat);
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, wrap);
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, wrap);
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                            static_cast<int>(TextureFilter::LinearMipmapLinear));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                            static_cast<int>(TextureFilter::Linear));
            if (m_MaxAnisotropy > 1.0f)
                m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy,
                                                m_MaxAnisotropy);

            tex->id = textureID;
            tex->width = data.width;
            tex->height = data.height;
            tex->nrComponents = data.nrComponents;
            if (keepCpuData)
                tex->SetPixelData(data.data.release(), Texture::PixelDataOwnership::Stbi);
            LOGGER_INFO("TextureManager") << "Async texture loaded: " << data.name;
            for (const auto& alias : aliases)
                EventManager::Instance().Publish(ResourceLoadedEvent{alias, "Texture", true});
            continue;
        }

        if (!data.data)
        {
            if (m_StrictLoading)
            {
                {
                    std::lock_guard<std::mutex> pathLock(m_DeduplicationMutex);
                    m_PathToTextureMap.erase(data.path);
                    m_PathReferenceCounts.erase(data.path);
                    for (auto it = m_NameToPathMap.begin(); it != m_NameToPathMap.end();)
                    {
                        if (it->second == data.path)
                        {
                            m_NameKeepCpuDataMap.erase(it->first);
                            it = m_NameToPathMap.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
                for (const auto& alias : aliases)
                    m_Cache.Remove(alias);
            }
            else if (tex && m_ErrorTexture)
            {
                tex->id = m_ErrorTexture->id;
                tex->width = m_ErrorTexture->width;
                tex->height = m_ErrorTexture->height;
                tex->nrComponents = m_ErrorTexture->nrComponents;
            }
            LOGGER_ERROR("TextureManager") << "Failed async texture: " << data.name;
            for (const auto& alias : aliases)
                EventManager::Instance().Publish(ResourceLoadedEvent{alias, "Texture", false});
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
    m_LoadGenerations.clear();
}

std::shared_ptr<Texture> TextureManager::CreateFromData(const std::string& name, const unsigned char* pixels, int width,
                                                        int height, int nrComponents, bool keepCpuData)
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
    m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format, DataType::UnsignedByte,
                                 pixels);
    m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);

    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, (int)TextureWrap::Repeat);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, (int)TextureWrap::Repeat);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                    (int)TextureFilter::LinearMipmapLinear);
    m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Linear);

    if (m_MaxAnisotropy > 1.0f)
    {
        m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy,
                                        m_MaxAnisotropy);
    }

    if (keepCpuData && pixels)
    {
        tex->SetPixelDataCopy(pixels, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                                          static_cast<std::size_t>(nrComponents));
    }

    m_Cache.Add(name, tex);
    return tex;
}
