#pragma once

#include <render/type/graphics_types.h>
#include <glm/glm.hpp>
#include <string>

class IShaderManager;


class ComputeShader
{
public:
    ComputeShader(IShaderManager& manager, std::string computePath);
    ~ComputeShader();

    void use();
    bool Reload();
    bool IsValid() const { return m_Program != 0; }
    unsigned int GetID() const { return m_Program; }
    const std::string& GetPath() const { return m_Path; }
    void Dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1,
                  MemoryBarrierBit barrier = MemoryBarrierBit::All);

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    IShaderManager& m_ShaderManager;
    std::string m_Path;
    unsigned int m_Program = 0;

    bool CheckCompileErrors(unsigned int object, const char* type) const;
};
