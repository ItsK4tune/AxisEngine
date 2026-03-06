#include <fstream>
#include <rendering/core/shader.h>
#include <rendering/interfaces/i_shader_manager.h>
#include <sstream>
#include <core/utils/logger.h>

namespace {
    constexpr unsigned int SHADER_VERTEX = 0x8B31;
    constexpr unsigned int SHADER_FRAGMENT = 0x8B30;
    constexpr unsigned int SHADER_GEOMETRY = 0x8DD9;
}

Shader::Shader(IShaderManager& manager)
    : ID(0), m_ShaderManager(manager)
{
}

Shader::Shader(IShaderManager& manager, const char *vertexPath, const char *fragmentPath, const char *geometryPath)
    : ID(0), m_ShaderManager(manager)
{
    load(vertexPath, fragmentPath, geometryPath);
}

void Shader::load(const char *vertexPath, const char *fragmentPath, const char *geometryPath)
{
    auto &sm = m_ShaderManager;

    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;

    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        if (geometryPath != nullptr)
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure &e)
    {
        LOGGER_ERROR("Shader") << "FILE_NOT_SUCCESFULLY_READ: " << e.what();
    }

    unsigned int vertex, fragment;

    vertex = sm.CreateShader(Graphics::ShaderType::Vertex);
    sm.ShaderSource(vertex, vertexCode.c_str());
    sm.CompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = sm.CreateShader(Graphics::ShaderType::Fragment);
    sm.ShaderSource(fragment, fragmentCode.c_str());
    sm.CompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    unsigned int geometry = 0;
    if (geometryPath != nullptr)
    {
        geometry = sm.CreateShader(Graphics::ShaderType::Geometry);
        sm.ShaderSource(geometry, geometryCode.c_str());
        sm.CompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    ID = sm.CreateProgram();
    sm.AttachShader(ID, vertex);
    sm.AttachShader(ID, fragment);
    if (geometryPath != nullptr)
        sm.AttachShader(ID, geometry);

    sm.LinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    sm.DeleteShader(vertex);
    sm.DeleteShader(fragment);
    if (geometryPath != nullptr)
        sm.DeleteShader(geometry);

    LOGGER_INFO("Shader") << "Shader loaded successfully: " << vertexPath << " | " << fragmentPath;
}

void Shader::use()
{
    m_ShaderManager.UseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const
{
    m_ShaderManager.SetUniform1i(GetUniformLocation(name), (int)value);
}

void Shader::setInt(const std::string &name, int value) const
{
    m_ShaderManager.SetUniform1i(GetUniformLocation(name), value);
}

void Shader::setFloat(const std::string &name, float value) const
{
    m_ShaderManager.SetUniform1f(GetUniformLocation(name), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const
{
    m_ShaderManager.SetUniform2fv(GetUniformLocation(name), &value[0]);
}

void Shader::setVec2(const std::string &name, float x, float y) const
{
    m_ShaderManager.SetUniform2f(GetUniformLocation(name), x, y);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const
{
    m_ShaderManager.SetUniform3fv(GetUniformLocation(name), &value[0]);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const
{
    m_ShaderManager.SetUniform3f(GetUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const
{
    m_ShaderManager.SetUniform4fv(GetUniformLocation(name), &value[0]);
}

void Shader::setVec4(const std::string &name, float x, float y, float z, float w)
{
    m_ShaderManager.SetUniform4f(GetUniformLocation(name), x, y, z, w);
}

void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const
{
    m_ShaderManager.SetUniformMatrix2fv(GetUniformLocation(name), &mat[0][0]);
}

void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const
{
    m_ShaderManager.SetUniformMatrix3fv(GetUniformLocation(name), &mat[0][0]);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    m_ShaderManager.SetUniformMatrix4fv(GetUniformLocation(name), &mat[0][0]);
}

void Shader::setMat4Array(const std::string &name, const std::vector<glm::mat4> &matrices) const
{
    if (matrices.empty()) return;
    int loc = GetUniformLocation(name);
    if (loc != -1)
    {
        int count = static_cast<int>(matrices.size());
        if (count > 128) count = 128;
        m_ShaderManager.SetUniformMatrix4fvArray(loc, count, &matrices[0][0][0]);
    }
}

int Shader::GetUniformLocation(const std::string &name) const
{
    if (uniformLocations.find(name) != uniformLocations.end())
        return uniformLocations[name];

    int location = m_ShaderManager.GetUniformLocation(ID, name.c_str());
    uniformLocations[name] = location;
    return location;
}

void Shader::checkCompileErrors(unsigned int shader, std::string type)
{
    auto &sm = m_ShaderManager;
    if (type != "PROGRAM")
    {
        if (!sm.GetShaderCompileStatus(shader))
        {
            std::string infoLog = sm.GetShaderInfoLog(shader);
            LOGGER_ERROR("Shader") << "COMPILATION_ERROR of type: " << type << "\n"
                                   << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    else
    {
        if (!sm.GetProgramLinkStatus(shader))
        {
            std::string infoLog = sm.GetProgramInfoLog(shader);
            LOGGER_ERROR("Shader") << "LINKING_ERROR of type: " << type << "\n"
                                   << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
}
