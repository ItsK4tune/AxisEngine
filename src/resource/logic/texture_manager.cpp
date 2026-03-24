#include <resource/logic/texture_manager.h>
#include <resource/type/resource_events.h>
#include <core/logic/job_system.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <stb/stb_image.h>

TextureManager::TextureManager(ITextureManager& lowLevelManager) 
    : m_LowLevelManager(lowLevelManager) {}

TextureManager::~TextureManager() {
    Clear();
}

std::shared_ptr<Texture> TextureManager::Load(const std::string& name, const std::string& path, bool async, bool keepCpuData) {
    if (auto existing = m_Cache.Get(name)) {
        return existing;
    }

    std::string fullPath = FileSystem::getPath(path);

    if (async && m_AsyncEnabled) {
        auto promise = std::make_shared<std::promise<TextureData>>();
        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_AsyncLoads.push_back(promise->get_future());
        }

        auto tex = std::make_shared<Texture>();
        tex->id = 0;
        tex->path = path;
        m_Cache.Add(name, tex);

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
    } else {
        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);

        if (data) {
            unsigned int textureID = m_LowLevelManager.GenTexture();
            
            TextureFormat format = TextureFormat::RGBA;
            InternalFormat iFormat = InternalFormat::RGBA8;

            if (nrComponents == 1) { format = TextureFormat::Red; iFormat = InternalFormat::R8; }
            else if (nrComponents == 3) { format = TextureFormat::RGB; iFormat = InternalFormat::RGB8; }
            else if (nrComponents == 4) { format = TextureFormat::RGBA; iFormat = InternalFormat::RGBA8; }

            m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
            m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, width, height, 0, format, DataType::UnsignedByte, data);
            m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);


            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(format == TextureFormat::RGBA ? TextureWrap::ClampToEdge : TextureWrap::Repeat));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::LinearMipmapLinear));
            m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

            if (m_MaxAnisotropy > 1.0f) {
                m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy, m_MaxAnisotropy);
            }

            auto tex = std::make_shared<Texture>();
            tex->id = textureID;
            tex->path = path;
            tex->width = width;
            tex->height = height;
            tex->nrComponents = nrComponents;
            tex->type = "texture_diffuse";

            if (keepCpuData) tex->pixelData = data;
            else stbi_image_free(data);

            m_Cache.Add(name, tex);
            LOGGER_INFO("TextureManager") << "Loaded texture: " << name;
            EventSystem::Instance().Publish(ResourceLoadedEvent{name, "Texture", true});
            return tex;
        } else {
            LOGGER_ERROR("TextureManager") << "Failed to load texture: " << path;
            EventSystem::Instance().Publish(ResourceLoadedEvent{name, "Texture", false});
            return nullptr;
        }
    }
}

std::shared_ptr<Texture> TextureManager::Get(const std::string& name) {
    return m_Cache.Get(name);
}

void TextureManager::Unload(const std::string& name) {
    if (auto tex = m_Cache.Get(name)) {
        if (tex->id != 0) {
            m_LowLevelManager.DeleteTextures(1, &tex->id);
        }
        m_Cache.Remove(name);
        LOGGER_INFO("TextureManager") << "Unloaded texture: " << name;
    }
}

void TextureManager::Update(float dt) {
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    auto it = m_AsyncLoads.begin();
    while (it != m_AsyncLoads.end()) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            TextureData data = it->get();
            if (data.data) {
                unsigned int textureID = m_LowLevelManager.GenTexture();

                TextureFormat format = TextureFormat::RGBA;
                InternalFormat iFormat = InternalFormat::RGBA8;
                if (data.nrComponents == 1) { format = TextureFormat::Red; iFormat = InternalFormat::R8; }
                else if (data.nrComponents == 3) { format = TextureFormat::RGB; iFormat = InternalFormat::RGB8; }
                else if (data.nrComponents == 4) { format = TextureFormat::RGBA; iFormat = InternalFormat::RGBA8; }

                m_LowLevelManager.BindTexture(TextureType::Texture2D, textureID);
                m_LowLevelManager.TexImage2D(TextureType::Texture2D, 0, iFormat, data.width, data.height, 0, format, DataType::UnsignedByte, data.data);
                m_LowLevelManager.GenerateMipmap(TextureType::Texture2D);
                m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::LinearMipmapLinear));
                m_LowLevelManager.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

                if (m_MaxAnisotropy > 1.0f) {
                    m_LowLevelManager.TexParameterf(TextureType::Texture2D, TextureParameter::TextureMaxAnisotropy, m_MaxAnisotropy);
                }

                if (auto tex = m_Cache.Get(data.name)) {
                    tex->id = textureID;
                    tex->width = data.width;
                    tex->height = data.height;
                    tex->nrComponents = data.nrComponents;
                    if (data.keepCpuData) tex->pixelData = data.data;
                    else stbi_image_free(data.data);
                }
                LOGGER_INFO("TextureManager") << "Async texture loaded: " << data.name;
                EventSystem::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", true});
            } else {
                LOGGER_ERROR("TextureManager") << "Failed async texture: " << data.name;
                EventSystem::Instance().Publish(ResourceLoadedEvent{data.name, "Texture", false});
            }
            it = m_AsyncLoads.erase(it);
        } else {
            ++it;
        }
    }
}

void TextureManager::Clear() {
    auto names = m_Cache.GetAllNames();
    for (const auto& name : names) {
        Unload(name);
    }
    m_Cache.Clear();
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    m_AsyncLoads.clear();
}
