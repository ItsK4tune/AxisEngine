#include <resource/texture_cache.h>
#include <interface/graphic/graphics_types.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <interface/graphic/i_texture_manager.h>
#include <stb_image.h>
#include <iostream>

ITextureManager* TextureCache::s_TextureManager = nullptr;

TextureCache::~TextureCache()
{
    Clear();
}

void TextureCache::LoadTexture(const std::string& name, const std::string& path, bool async)
{
    std::string fullPath = FileSystem::getPath(path);

    if (async)
    {
        m_AsyncLoads.push_back(std::async(std::launch::async, [name, fullPath]() -> TextureData {
            TextureData data;
            data.name = name;
            data.path = fullPath;
            stbi_set_flip_vertically_on_load(true);
            data.data = stbi_load(fullPath.c_str(), &data.width, &data.height, &data.nrComponents, 0);
            return data;
        }));
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

            Graphics::TextureFormat format = Graphics::TextureFormat::RGBA;
            Graphics::InternalFormat internalFormat = Graphics::InternalFormat::RGBA8;

            if (nrComponents == 1) {
                format = Graphics::TextureFormat::Red;
                const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGB8;
            } else if (nrComponents == 3) {
                format = Graphics::TextureFormat::RGB;
                const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGB8;
            } else if (nrComponents == 4) {
                format = Graphics::TextureFormat::RGBA;
                const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGBA8;
            }

            tm.BindTexture(Graphics::TextureType::Texture2D, textureID);

            Graphics::InternalFormat iFormat = Graphics::InternalFormat::RGBA8;
            if (nrComponents == 1) iFormat = Graphics::InternalFormat::RGB8;
            else if (nrComponents == 3) iFormat = Graphics::InternalFormat::RGB8;
            else if (nrComponents == 4) iFormat = Graphics::InternalFormat::RGBA8;

            tm.TexImage2D(Graphics::TextureType::Texture2D, 0, iFormat, width, height, 0, format, Graphics::DataType::UnsignedByte, data);
            tm.GenerateMipmap(Graphics::TextureType::Texture2D);

            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(format == Graphics::TextureFormat::RGBA ? Graphics::TextureWrap::ClampToEdge : Graphics::TextureWrap::Repeat));
            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(format == Graphics::TextureFormat::RGBA ? Graphics::TextureWrap::ClampToEdge : Graphics::TextureWrap::Repeat));
            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::LinearMipmapLinear));
            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));

            stbi_image_free(data);

            auto tex = std::make_shared<Texture>();
            tex->id = textureID;
            tex->type = "texture_diffuse";
            tex->path = path;
            m_Textures[name] = tex;

            LOGGER_INFO("TextureCache") << "Loaded texture: " << name;
        }
        else
        {
            LOGGER_ERROR("TextureCache") << "Failed to load texture: " << path;
            stbi_image_free(data);
        }
    }
}

std::shared_ptr<Texture> TextureCache::GetTexture(const std::string& name)
{
    if (m_Textures.find(name) != m_Textures.end())
        return m_Textures[name];
    return nullptr;
}

void TextureCache::UnloadTexture(const std::string& name)
{
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
    {
        GetTextureManager().DeleteTextures(1, &it->second->id);
        m_Textures.erase(it);
        LOGGER_INFO("TextureCache") << "Unloaded texture: " << name;
    }
}

bool TextureCache::IsTextureLoaded(const std::string& name) const
{
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

                Graphics::TextureFormat format = Graphics::TextureFormat::RGBA;
                Graphics::InternalFormat iFormat = Graphics::InternalFormat::RGBA8;

                if (data.nrComponents == 1) {
                    format = Graphics::TextureFormat::Red;
                    iFormat = Graphics::InternalFormat::RGB8;
                } else if (data.nrComponents == 3) {
                    format = Graphics::TextureFormat::RGB;
                    iFormat = Graphics::InternalFormat::RGB8;
                } else if (data.nrComponents == 4) {
                    format = Graphics::TextureFormat::RGBA;
                    iFormat = Graphics::InternalFormat::RGBA8;
                }

                tm.BindTexture(Graphics::TextureType::Texture2D, textureID);
                tm.TexImage2D(Graphics::TextureType::Texture2D, 0, iFormat, data.width, data.height, 0, format, Graphics::DataType::UnsignedByte, data.data);
                tm.GenerateMipmap(Graphics::TextureType::Texture2D);

                tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(format == Graphics::TextureFormat::RGBA ? Graphics::TextureWrap::ClampToEdge : Graphics::TextureWrap::Repeat));
                tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(format == Graphics::TextureFormat::RGBA ? Graphics::TextureWrap::ClampToEdge : Graphics::TextureWrap::Repeat));
                tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::LinearMipmapLinear));
                tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));

                stbi_image_free(data.data);

                auto tex = std::make_shared<Texture>();
                tex->id = textureID;
                tex->type = "texture_diffuse";
                tex->path = data.path;
                m_Textures[data.name] = tex;

                LOGGER_INFO("TextureCache") << "Async texture loaded: " << data.name;
            }
            else
            {
                LOGGER_ERROR("TextureCache") << "Failed to async load texture: " << data.path;
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
    for (auto& pair : m_Textures)
    {
        GetTextureManager().DeleteTextures(1, &pair.second->id);
    }
    m_Textures.clear();
    m_AsyncLoads.clear();
}
