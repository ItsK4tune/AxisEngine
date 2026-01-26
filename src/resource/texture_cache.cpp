#include <resource/texture_cache.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

TextureCache::TextureCache()
{
}

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
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
        
        if (data)
        {
            GLenum format = GL_RGBA;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

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

Texture* TextureCache::GetTexture(const std::string& name)
{
    if (m_Textures.find(name) != m_Textures.end())
        return &m_Textures[name];
    return nullptr;
}

void TextureCache::UnloadTexture(const std::string& name)
{
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
    {
        glDeleteTextures(1, &it->second.id);
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
                unsigned int textureID;
                glGenTextures(1, &textureID);

                GLenum format = GL_RGBA;
                if (data.nrComponents == 1)
                    format = GL_RED;
                else if (data.nrComponents == 3)
                    format = GL_RGB;
                else if (data.nrComponents == 4)
                    format = GL_RGBA;

                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                stbi_image_free(data.data);

                Texture tex;
                tex.id = textureID;
                tex.type = "texture_diffuse";
                tex.path = data.path;
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
        glDeleteTextures(1, &pair.second.id);
    }
    m_Textures.clear();
    m_AsyncLoads.clear();
}
