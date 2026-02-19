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

    Shader* GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader* Get(const std::string& name);
    
    void Reload(const std::string& name);

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_LoadedShaders;
};
