#include <resource/logic/skybox_manager.h>
#include <core/logic/logger.h>

std::shared_ptr<Skybox> SkyboxManager::Load(const std::string& name, const std::vector<std::string>& faces) {
    if (auto existing = m_Cache.Get(name)) return existing;

    auto skybox = std::make_shared<Skybox>();
    skybox->SetName(name);
    skybox->LoadCubemap(faces);
    if (skybox) {
        m_Cache.Add(name, skybox);
        LOGGER_INFO("SkyboxManager") << "Loaded skybox: " << name;
        return skybox;
    }
    
    LOGGER_ERROR("SkyboxManager") << "Failed to load skybox: " << name;
    return nullptr;
}

std::shared_ptr<Skybox> SkyboxManager::Get(const std::string& nameOrPath) {
    return m_Cache.Get(nameOrPath);
}

void SkyboxManager::Unload(const std::string& nameOrPath) {
    m_Cache.Remove(nameOrPath);
}

void SkyboxManager::Clear() {
    m_Cache.Clear();
}
