#pragma once

#include <memory>
#include <render/logic/shader.h>
#include <string>
#include <unordered_map>

class ShaderCache
{
public:
    ShaderCache() = delete;
    explicit ShaderCache(IShaderManager& manager);

    std::shared_ptr<Shader> GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<Shader> GetShared(const std::string& name);
    Shader* Get(const std::string& name);
    void Remove(const std::string& name) { m_LoadedShaders.erase(name); }
    void Reload(const std::string& name);

private:
    IShaderManager& m_ShaderManager;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_LoadedShaders;
};