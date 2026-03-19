#pragma once

#include <memory>
#include <string>

class Shader;

class IShaderLibrary
{
public:
    virtual ~IShaderLibrary() = default;
    virtual void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "") = 0;
    virtual void UnloadShader(const std::string& name) = 0;
    virtual std::shared_ptr<Shader> GetShader(const std::string& name) = 0;
};