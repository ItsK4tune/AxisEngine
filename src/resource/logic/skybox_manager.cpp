#include <resource/logic/skybox_manager.h>
#include <core/logic/logger.h>

std::shared_ptr<Skybox> SkyboxManager::Load(const std::string& name, const std::vector<std::string>& faces)
{
    if (auto existing = m_Cache.Get(name))
        return existing;

    auto skybox = std::make_shared<Skybox>();
    skybox->SetName(name);
    const bool allFacesLoaded = skybox->LoadCubemap(faces);
    if (skybox->GetTextureID() == 0)
    {
        LOGGER_ERROR("SkyboxManager") << "Failed to create skybox texture: " << name;
        return nullptr;
    }
    m_Cache.Add(name, skybox);
    if (allFacesLoaded)
        LOGGER_INFO("SkyboxManager") << "Loaded skybox: " << name;
    else
        LOGGER_WARN("SkyboxManager") << "Loaded skybox with fallback faces: " << name;
    return skybox;
}

std::shared_ptr<Skybox> SkyboxManager::Get(const std::string& nameOrPath)
{
    return m_Cache.Get(nameOrPath);
}

void SkyboxManager::Unload(const std::string& nameOrPath)
{
    m_Cache.Remove(nameOrPath);
}

void SkyboxManager::Clear()
{
    m_Cache.Clear();
}
