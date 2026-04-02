#include <resource/logic/shader_manager.h>
#include <core/logic/logger.h>
#include <render/interface/i_graphics_context.h>
#include <core/logic/service_locator.h>
#include <core/logic/filesystem.h>
#include <ecs/interface/i_geometry_service.h>

ShaderManager::ShaderManager(IShaderManager& lowLevelManager) 
    : m_LowLevelManager(lowLevelManager) {}

void ShaderManager::Initialize() {
    auto loadErrorShader = [&](const std::string& name, const std::string& vsName, const std::string& fsName) {
        auto shader = std::make_shared<Shader>(m_LowLevelManager);
        shader->SetName(name);
        
        // Construct full paths relative to the engine's asset directory
        std::string vsPath = FileSystem::getPath("include/engine/asset/shaders/" + vsName);
        std::string fsPath = FileSystem::getPath("include/engine/asset/shaders/" + fsName);
        
        try {
            shader->load(vsPath.c_str(), fsPath.c_str());
        } catch (...) {
            LOGGER_ERROR("ShaderManager") << "CRITICAL: Failed to load external error shader: " << name;
        }
        return shader;
    };

    m_ErrorShader = loadErrorShader("Internal_Error_Shader", "error_forward.vs", "error_forward.fs");
    m_ErrorGBufferShader = loadErrorShader("Internal_Error_GBuffer_Shader", "error_deferred.vs", "error_deferred.fs");

    m_Cache.Add("Internal_Error_Shader", m_ErrorShader);
    m_Cache.Add("Internal_Error_GBuffer_Shader", m_ErrorGBufferShader);

    LOGGER_INFO("ShaderManager") << "Initialized Externalized Error Shaders (Pink/Black Checkerboard)";
}

std::shared_ptr<Shader> ShaderManager::Load(const std::string& name, 
                                            const std::string& vsPath, 
                                            const std::string& fsPath, 
                                            const std::string& gsPath) {
    if (auto existing = m_Cache.Get(name)) {
        return existing;
    }

    if (vsPath.empty() || fsPath.empty()) {
        LOGGER_ERROR("ShaderManager") << "Empty shader paths for '" << name << "'. Returning error shader.";
        return m_ErrorShader;
    }

    auto shader = std::make_shared<Shader>(m_LowLevelManager);
    shader->SetName(name);
    
    try {
        shader->load(vsPath.c_str(), fsPath.c_str(), gsPath.empty() ? nullptr : gsPath.c_str());
    } catch (...) {
        shader->SetError(true);
    }
    
    if (shader->IsError()) {
        LOGGER_ERROR("ShaderManager") << "Failed to load shader: " << name << ". Returning error shader.";
        m_Cache.Add(name, m_ErrorShader); // Add the error shader to cache for this name
        m_Paths[name] = {vsPath, fsPath, gsPath}; // Store paths for potential reload
        return m_ErrorShader; // Return the error shader
    }
    
    m_Cache.Add(name, shader);
    m_Paths[name] = {vsPath, fsPath, gsPath};
    
    LOGGER_INFO("ShaderManager") << "Loaded and cached shader: " << name;
    
    return shader;
}

std::shared_ptr<Shader> ShaderManager::Get(const std::string& nameOrPath) {
    return m_Cache.Get(nameOrPath);
}

void ShaderManager::Unload(const std::string& nameOrPath) {
    m_Cache.Remove(nameOrPath);
    m_Paths.erase(nameOrPath);
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
