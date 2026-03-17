#include <resource/manager/skybox_manager.h>
#include <core/logic/logger.h>

std::shared_ptr<Skybox> SkyboxManager::Load(const std::string& name, const std::vector<std::string>& faces) {
    if (auto existing = m_Cache.Get(name)) return existing;

    auto skybox = std::make_shared<Skybox>();
    skybox->LoadCubemap(faces);
    if (skybox) {
        m_Cache.Add(name, skybox);
        LOGGER_INFO("SkyboxManager") << "Loaded skybox: " << name;
        return skybox;
    }
    
    LOGGER_ERROR("SkyboxManager") << "Failed to load skybox: " << name;
    return nullptr;
}

std::shared_ptr<Skybox> SkyboxManager::Get(const std::string& name) {
    return m_Cache.Get(name);
}

void SkyboxManager::Unload(const std::string& name) {
    m_Cache.Remove(name);
}
