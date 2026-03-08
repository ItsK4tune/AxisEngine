#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class IShaderManager;

#define GLM_ENABLE_EXPERIMENTAL


class Shader
{
public:
    unsigned int ID;

    Shader() = delete;
    explicit Shader(IShaderManager& manager);
    Shader(IShaderManager& manager, const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);

    void load(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);
    void use();

    void setID(unsigned int id) { ID = id; }
    unsigned int getID() const { return ID; }
    std::string GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w);
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setMat4Array(const std::string &name, const std::vector<glm::mat4> &matrices) const;

    int GetUniformLocation(const std::string &name) const;


private:
    void checkCompileErrors(unsigned int shader, std::string type);
    mutable std::unordered_map<std::string, int> uniformLocations;

    IShaderManager& m_ShaderManager;
    std::string m_Name;
};