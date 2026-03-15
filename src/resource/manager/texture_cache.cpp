#include <core/logic/job_system.h>
#include <render/type/graphics_types.h>
#include <render/interface/i_texture_manager.h>
#include <iostream>
#include <resource/logic/texture_cache.h>
#include <stb/stb_image.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/event_system.h>
#include <core/logic/event_types.h>
ITextureManager* TextureCache::s_TextureManager = nullptr;
bool TextureCache::s_AsyncEnabled = true;
float TextureCache::s_MaxAnisotropy = 1.0f;

TextureCache::~TextureCache()
{
    Clear();
}

void TextureCache::LoadTexture(const std::string& name, const std::string& path, bool async, bool keepCpuData)
{
    std::string fullPath = FileSystem::getPath(path);
    
    if (async && s_AsyncEnabled)
    {
        auto promise = std::make_shared<std::promise<TextureData>>();
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_AsyncLoads.push_back(promise->get_future());
        }

        // Create resource entry immediately so GetTexture returns it while loading
        auto tex = std::make_shared<Texture>();
        tex->id = 0; // Will be set in Update()
        tex->path = path;
        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            m_Textures[name] = tex;
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
    }
    else
    {
        auto& tm = GetTextureManager();
        unsigned int textureID = tm.GenTexture();

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            TextureFormat format = TextureFormat::RGBA;
            InternalFormat iFormat = InternalFormat::RGBA8;

            if (nrComponents == 1) {
                format = TextureFormat::Red;
                iFormat = InternalFormat::R8;
            }
            else if (nrComponents == 3) {
                format = TextureFormat::RGB;
                iFormat = InternalFormat::RGB8;
            }
            else if (nrComponents == 4) {
                format = TextureFormat::RGBA;
                iFormat = InternalFormat::RGBA8;
            }

            tm.BindTexture(TextureType::Texture2D, textureID);
            tm.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format, DataType::UnsignedByte, data);
            tm.GenerateMipmap(TextureType::Texture2D);

            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::LinearMipmapLinear));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

            auto tex = std::make_shared<Texture>();
            tex->id = textureID;
            tex->type = "texture_diffuse";
            tex->path = path;
            tex->width = width;
            tex->height = height;
            tex->nrComponents = nrComponents;

            if (keepCpuData) {
                tex->pixelData = data;
            } else {
                stbi_image_free(data);
            }

            {
                std::lock_guard<std::mutex> lock(m_CacheMutex);
                m_Textures[name] = tex;
            }

            LOGGER_INFO("TextureCache") << "Loaded Texture: " << name << " (" << width << "x" << height << ", " << nrComponents << " channels)" << (keepCpuData ? " (CPU data kept)" : "");
            EventSystem::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
        }
        else
        {
            LOGGER_ERROR("TextureCache") << "Failed to load Texture: " << path;
            EventSystem::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
        }
    }
}

std::shared_ptr<Texture> TextureCache::GetTexture(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    if (m_Textures.find(name) != m_Textures.end())
        return m_Textures[name];
    return nullptr;
}

void TextureCache::UnloadTexture(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
    {
        GetTextureManager().DeleteTextures(1, &it->second->id);
        m_Textures.erase(it);
        LOGGER_INFO("TextureCache") << "Unloaded Texture: " << name;
    }
}

bool TextureCache::IsTextureLoaded(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    return m_Textures.find(name) != m_Textures.end();
}

void TextureCache::Update()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_AsyncLoads.begin();
    while (it != m_AsyncLoads.end())
    {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            TextureData data = it->get();
            if (data.data)
            {
                auto& tm = GetTextureManager();
                unsigned int textureID = tm.GenTexture();

                TextureFormat format = TextureFormat::RGBA;
                InternalFormat iFormat = InternalFormat::RGBA8;

                if (data.nrComponents == 1) {
                    format = TextureFormat::Red;
                    iFormat = InternalFormat::R8;
                } else if (data.nrComponents == 3) {
                    format = TextureFormat::RGB;
                    iFormat = InternalFormat::RGB8;
                } else if (data.nrComponents == 4) {
                    format = TextureFormat::RGBA;
                    iFormat = InternalFormat::RGBA8;
                }

                tm.BindTexture(TextureType::Texture2D, textureID);
                tm.TexImage2D(TextureType::Texture2D, 0, iFormat, data.width, data.height, 0, format, DataType::UnsignedByte, data.data);
                tm.GenerateMipmap(TextureType::Texture2D);

                tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
                tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
                tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::LinearMipmapLinear));
                tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

                // Find existing texture object to update
                std::shared_ptr<Texture> tex = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_CacheMutex);
                    if (m_Textures.find(data.name) != m_Textures.end())
                        tex = m_Textures[data.name];
                }

                if (tex)
                {
                    tex->id = textureID;
                    tex->width = data.width;
                    tex->height = data.height;
                    tex->nrComponents = data.nrComponents;
                    if (data.keepCpuData) tex->pixelData = data.data;
                    else stbi_image_free(data.data);
                }

                LOGGER_INFO("TextureCache") << "Async Texture loaded: " << data.name << " (" << data.width << "x" << data.height << ", " << data.nrComponents << " channels)" << (data.keepCpuData ? " (CPU data kept)" : "");
                EventSystem::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", true});
            }
            else
            {
                LOGGER_WARN("TextureCache") << "Failed to async load Texture: " << data.path;
                EventSystem::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", false});
            }

            it = m_AsyncLoads.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TextureCache::Clear()
{
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    for (auto& pair : m_Textures)
    {
        GetTextureManager().DeleteTextures(1, &pair.second->id);
    }
    m_Textures.clear();
    m_AsyncLoads.clear();
}