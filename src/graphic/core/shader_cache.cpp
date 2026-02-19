#include <graphic/core/shader_cache.h>
#include <utils/logger.h>
#include <iostream>

Shader* ShaderCache::GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath)
{
    if (m_LoadedShaders.find(name) != m_LoadedShaders.end())
    {
        return m_LoadedShaders[name].get();
    }
    
    if (vertPath.empty() || fragPath.empty())
    {
        LOGGER_ERROR("ShaderCache") << "Empty shader paths for '" << name << "'!";
        return nullptr;
    }

    auto shader = std::make_unique<Shader>();
    shader->load(vertPath.c_str(), fragPath.c_str());
    
    Shader* rawPtr = shader.get();
    m_LoadedShaders[name] = std::move(shader);
    
    LOGGER_INFO("ShaderCache") << "Compiled and cached shader '" << name << "'";
    
    return rawPtr;
}

Shader* ShaderCache::Get(const std::string& name)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
        return it->second.get();
    return nullptr;
}

void ShaderCache::Reload(const std::string& name)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
    {
        LOGGER_INFO("ShaderCache") << "Reloading shader: " << name;
        
        
        
        
        m_LoadedShaders.erase(it);
    }
}
