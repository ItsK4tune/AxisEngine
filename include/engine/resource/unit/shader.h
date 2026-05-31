#pragma once

#include <render/type/graphics_types.h>
#include <glm/glm.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class IShaderManager;

#define GLM_ENABLE_EXPERIMENTAL

class Shader
{
public:
    unsigned int ID;

    using UniformValue =
        std::variant<bool, int, unsigned int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

    Shader() = delete;
    explicit Shader(IShaderManager& manager);
    Shader(IShaderManager& manager, const char* vertexPath, const char* fragmentPath,
           const char* geometryPath = nullptr);

    void load(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    void use();

    void setID(unsigned int id)
    {
        ID = id;
    }
    unsigned int getID() const
    {
        return ID;
    }
    const std::string& GetName() const
    {
        return m_Name;
    }
    void SetName(const std::string& name)
    {
        m_Name = name;
    }

    // Common uniform locations cached for fast access
    mutable int m_Loc_u_Model = -2;
    mutable int m_Loc_u_TintColor = -2;
    mutable int m_Loc_u_EntityID = -2;
    mutable int m_Loc_u_IsInstanced = -2;
    mutable int m_Loc_u_ReceiveShadowFlag = -2;
    mutable int m_Loc_u_ProbeIndex = -2;
    mutable int m_Loc_u_ProbePos = -2;
    mutable int m_Loc_u_ProbeBoxMin = -2;
    mutable int m_Loc_u_ProbeBoxMax = -2;
    mutable int m_Loc_u_ReflectionProbe = -2;
    mutable int m_Loc_u_HasReflection = -2;
    mutable int m_Loc_u_HasProbe = -2;
    mutable int m_Loc_u_Reflectivity = -2;
    mutable int m_Loc_u_FresnelPower = -2;
    mutable int m_Loc_u_FresnelBias = -2;
    mutable int m_Loc_u_ReflectionIntensity = -2;
    mutable int m_Loc_isInstanced = -2;

    bool IsError() const
    {
        return m_IsError;
    }
    void SetError(bool error)
    {
        m_IsError = error;
    }

    bool IsDeferred() const
    {
        return m_IsDeferred;
    }
    void SetDeferred(bool deferred)
    {
        m_IsDeferred = deferred;
    }

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setUInt(const std::string& name, unsigned int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3Array(const std::string& name, const glm::vec3* values, int count) const;
    void setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const;
    void setCustomPorts(const ShaderPorts& ports) const;

    void setBool(int location, bool value) const;
    void setInt(int location, int value) const;
    void setUInt(int location, unsigned int value) const;
    void setFloat(int location, float value) const;
    void setVec2(int location, const glm::vec2& value) const;
    void setVec3(int location, const glm::vec3& value) const;
    void setVec4(int location, const glm::vec4& value) const;
    void setMat4(int location, const glm::mat4& mat) const;
    void setVec3Array(int location, const glm::vec3* values, int count) const;
    void setMat4Array(int location, const std::vector<glm::mat4>& matrices) const;

    int GetUniformLocation(const std::string& name) const;

    void setBool_Fast(int& cachedLoc, const std::string& name, bool value) const;
    void setInt_Fast(int& cachedLoc, const std::string& name, int value) const;
    void setUInt_Fast(int& cachedLoc, const std::string& name, unsigned int value) const;
    void setFloat_Fast(int& cachedLoc, const std::string& name, float value) const;
    void setVec3_Fast(int& cachedLoc, const std::string& name, const glm::vec3& value) const;
    void setVec4_Fast(int& cachedLoc, const std::string& name, const glm::vec4& value) const;
    void setMat4_Fast(int& cachedLoc, const std::string& name, const glm::mat4& mat) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type);

    mutable std::unordered_map<std::string, int> m_UniformLocations;
    mutable std::mutex m_UniformMutex;

    IShaderManager& m_ShaderManager;
    std::string m_Name;
    bool m_IsError = false;
    bool m_IsDeferred = false;
};
