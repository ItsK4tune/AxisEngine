#pragma once

#include <glm/glm.hpp>
#include <render/type/graphics_types.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <mutex>

class IShaderManager;

#define GLM_ENABLE_EXPERIMENTAL

class Shader
{
public:
    unsigned int ID;

    using UniformValue = std::variant<bool, int, unsigned int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

    Shader() = delete;
    explicit Shader(IShaderManager& manager);
    Shader(IShaderManager& manager, const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);

    void load(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);
    void use();

    void setID(unsigned int id) { ID = id; }
    unsigned int getID() const { return ID; }
    std::string GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }
    
    bool IsError() const { return m_IsError; }
    void SetError(bool error) { m_IsError = error; }


    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setUInt(const std::string &name, unsigned int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setVec4(const std::string &name, const glm::vec4 &value) const;
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    void setMat2(const std::string &name, const glm::mat2 &mat) const;
    void setMat3(const std::string &name, const glm::mat3 &mat) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setMat4Array(const std::string &name, const std::vector<glm::mat4> &matrices) const;
    void setCustomPorts(const ShaderPorts& ports) const;


    void setBool(int location, bool value) const;
    void setInt(int location, int value) const;
    void setUInt(int location, unsigned int value) const;
    void setFloat(int location, float value) const;
    void setVec2(int location, const glm::vec2 &value) const;
    void setVec3(int location, const glm::vec3 &value) const;
    void setVec4(int location, const glm::vec4 &value) const;
    void setMat4(int location, const glm::mat4 &mat) const;
    void setMat4Array(int location, const std::vector<glm::mat4> &matrices) const;
    
    int GetUniformLocation(const std::string &name) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
    

    mutable std::unordered_map<std::string, int> m_UniformLocations;
    mutable std::mutex m_UniformMutex;

    IShaderManager& m_ShaderManager;
    std::string m_Name;
    bool m_IsError = false;
};
