#include <graphic/shader_cache.h>
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
        std::cerr << "[ShaderCache] ERROR: Empty shader paths for '" << name << "'!" << std::endl;
        return nullptr;
    }

    Shader* shader = new Shader();
    shader->load(vertPath.c_str(), fragPath.c_str());
    
    m_LoadedShaders[name] = shader;
    std::cout << "[ShaderCache] Compiled and cached shader '" << name << "'" << std::endl;
    
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
        std::cout << "[ShaderCache] Reloading shader: " << name << std::endl;
        delete it->second;
        m_LoadedShaders.erase(it);
    }
}
