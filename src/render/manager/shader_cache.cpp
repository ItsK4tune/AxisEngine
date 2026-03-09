#include <render/interface/i_shader_manager.h>
#include <render/logic/shader_cache.h>
#include <iostream>
#include <core/logic/logger.h>

ShaderCache::ShaderCache(IShaderManager& manager) : m_ShaderManager(manager) {}

std::shared_ptr<Shader> ShaderCache::GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath, const std::string& geomPath)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
        return it->second;

    if (vertPath.empty() || fragPath.empty())
    {
        LOGGER_ERROR("ShaderCache") << "Empty shader paths for '" << name << "'!";
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(m_ShaderManager);
    shader->SetName(name);
    shader->load(vertPath.c_str(), fragPath.c_str(), geomPath.empty() ? nullptr : geomPath.c_str());

    m_LoadedShaders[name] = shader;

    LOGGER_INFO("ShaderCache") << "Compiled and cached shader '" << name << "'";

    return shader;
}

std::shared_ptr<Shader> ShaderCache::GetShared(const std::string& name)
{
    auto it = m_LoadedShaders.find(name);
    if (it != m_LoadedShaders.end())
        return it->second;
    return nullptr;
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
