#include <resource/logic/shader_manager.h>
#include <core/logic/logger.h>

ShaderManager::ShaderManager(IShaderManager& lowLevelManager) 
    : m_LowLevelManager(lowLevelManager) {}

std::shared_ptr<Shader> ShaderManager::Load(const std::string& name, 
                                            const std::string& vsPath, 
                                            const std::string& fsPath, 
                                            const std::string& gsPath) {
    if (auto existing = m_Cache.Get(name)) {
        return existing;
    }

    if (vsPath.empty() || fsPath.empty()) {
        LOGGER_ERROR("ShaderManager") << "Empty shader paths for '" << name << "'!";
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(m_LowLevelManager);
    shader->SetName(name);
    
    // Shader::load returns void and handles its own errors via logging/exceptions
    shader->load(vsPath.c_str(), fsPath.c_str(), gsPath.empty() ? nullptr : gsPath.c_str());
    
    m_Cache.Add(name, shader);
    m_Paths[name] = {vsPath, fsPath, gsPath};
    
    LOGGER_INFO("ShaderManager") << "Loaded and cached shader: " << name;
    
    return shader;
}

std::shared_ptr<Shader> ShaderManager::Get(const std::string& name) {
    return m_Cache.Get(name);
}

void ShaderManager::Unload(const std::string& name) {
    m_Cache.Remove(name);
    m_Paths.erase(name);
}

void ShaderManager::Reload(const std::string& name) {
    auto it = m_Paths.find(name);
    if (it != m_Paths.end()) {
        LOGGER_INFO("ShaderManager") << "Reloading shader: " << name;
        auto shader = std::make_shared<Shader>(m_LowLevelManager);
        shader->SetName(name);
        shader->load(it->second.vs.c_str(), it->second.fs.c_str(), 
                         it->second.gs.empty() ? nullptr : it->second.gs.c_str());
        m_Cache.Add(name, shader);
    }
}

void ShaderManager::ReloadAll() {
    auto names = m_Cache.GetAllNames();
    for (const auto& name : names) {
        Reload(name);
    }
}

void ShaderManager::Clear() {
    m_Cache.Clear();
    m_Paths.clear();
}
