#pragma once

#include <graphic/core/shader.h>
#include <string>
#include <unordered_map>
#include <memory>

class ShaderCache
{
public:
    ShaderCache() = default;
    ~ShaderCache() = default;

    std::shared_ptr<Shader> GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<Shader> GetShared(const std::string& name);
    Shader* Get(const std::string& name);
    void Remove(const std::string& name) { m_LoadedShaders.erase(name); }
    void Reload(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_LoadedShaders;
};
