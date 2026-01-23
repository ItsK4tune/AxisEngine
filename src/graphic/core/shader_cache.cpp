#include <graphic/core/shader_cache.h>
#include <utils/logger.h>
#include <iostream>

ShaderCache::ShaderCache()
{
}

ShaderCache::~ShaderCache()
{
    for (auto& pair : m_LoadedShaders)
    {
        delete pair.second;
    }
    m_LoadedShaders.clear();
}

Shader* ShaderCache::GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath)
{
    if (m_LoadedShaders.find(name) != m_LoadedShaders.end())
    {
        return m_LoadedShaders[name];
    }
    
    if (vertPath.empty() || fragPath.empty())
    {
        LOGGER_ERROR("ShaderCache") << "Empty shader paths for '" << name << "'!";
        return nullptr;
    }

    Shader* shader = new Shader();
    shader->load(vertPath.c_str(), fragPath.c_str());
    
    m_LoadedShaders[name] = shader;
    LOGGER_INFO("ShaderCache") << "Compiled and cached shader '" << name << "'";
    
    return shader;
}

Shader* ShaderCache::Get(const std::string& name)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
        return it->second;
    return nullptr;
}

void ShaderCache::Reload(const std::string& name)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
    {
        LOGGER_INFO("ShaderCache") << "Reloading shader: " << name;
        delete it->second;
        m_LoadedShaders.erase(it);
    }
}
