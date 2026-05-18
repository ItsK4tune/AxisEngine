#include <resource/unit/shader.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_geometry_service.h>
#include <render/interface/i_shader_manager.h>
#include <resource/logic/shader_manager.h>
#include <fstream>
#include <sstream>

Shader::Shader(IShaderManager& manager) : ID(0), m_ShaderManager(manager)
{
}

Shader::Shader(IShaderManager& manager, const char* vertexPath, const char* fragmentPath, const char* geometryPath)
    : ID(0), m_ShaderManager(manager)
{
    load(vertexPath, fragmentPath, geometryPath);
}

void Shader::load(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    auto& sm = m_ShaderManager;

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
    catch (std::ifstream::failure& e)
    {
        LOGGER_ERROR("Shader") << "FILE_NOT_SUCCESFULLY_READ: " << e.what();
    }

    unsigned int Vertex, fragment;

    Vertex = sm.CreateShader(ShaderType::Vertex);
    sm.ShaderSource(Vertex, vertexCode.c_str());
    sm.CompileShader(Vertex);
    checkCompileErrors(Vertex, "Vertex");

    fragment = sm.CreateShader(ShaderType::Fragment);
    sm.ShaderSource(fragment, fragmentCode.c_str());
    sm.CompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    unsigned int geometry = 0;
    if (geometryPath != nullptr)
    {
        geometry = sm.CreateShader(ShaderType::Geometry);
        sm.ShaderSource(geometry, geometryCode.c_str());
        sm.CompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
    }

    ID = sm.CreateProgram();
    sm.AttachShader(ID, Vertex);
    sm.AttachShader(ID, fragment);
    if (geometryPath != nullptr)
        sm.AttachShader(ID, geometry);

    sm.LinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    sm.DeleteShader(Vertex);
    sm.DeleteShader(fragment);
    if (geometryPath != nullptr)
        sm.DeleteShader(geometry);

    {
        std::lock_guard<std::mutex> lock(m_UniformMutex);
        m_UniformLocations.clear();
    }

    LOGGER_INFO("Shader") << "Shader loaded successfully: " << vertexPath << " | " << fragmentPath;
}

void Shader::use()
{
    if (m_IsError)
    {
        auto& sl = ServiceLocator::Instance();
        auto& sm = sl.Require<ShaderManager>();
        auto* gs = sl.Resolve<IGeometryService>();

        bool isDeferred = gs && gs->IsDeferredRenderingEnabled();

        std::shared_ptr<Shader> fallback;
        if (isDeferred)
        {
            fallback = sm.Get("Internal_Error_GBuffer_Shader");
        }
        else
        {
            fallback = sm.Get("Internal_Error_Shader");
        }

        if (fallback && fallback.get() != this && !fallback->m_IsError)
        {
            fallback->use();
            return;
        }
    }

    if (ID != 0)
    {
        m_ShaderManager.UseProgram(ID);
    }
}

void Shader::setBool(const std::string& name, bool value) const
{
    setBool(GetUniformLocation(name), value);
}

void Shader::setBool(int location, bool value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform1i(location, (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
    setInt(GetUniformLocation(name), value);
}

void Shader::setInt(int location, int value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform1i(location, value);
}

void Shader::setUInt(const std::string& name, unsigned int value) const
{
    setUInt(GetUniformLocation(name), value);
}

void Shader::setUInt(int location, unsigned int value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform1ui(location, value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    setFloat(GetUniformLocation(name), value);
}

void Shader::setFloat(int location, float value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform1f(location, value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    setVec2(GetUniformLocation(name), value);
}

void Shader::setVec2(int location, const glm::vec2& value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform2fv(location, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    setVec2(name, glm::vec2(x, y));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    setVec3(GetUniformLocation(name), value);
}

void Shader::setVec3(int location, const glm::vec3& value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform3fv(location, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    setVec3(name, glm::vec3(x, y, z));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    setVec4(GetUniformLocation(name), value);
}

void Shader::setVec4(int location, const glm::vec4& value) const
{
    if (location != -1)
        m_ShaderManager.SetUniform4fv(location, &value[0]);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
    setVec4(name, glm::vec4(x, y, z, w));
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    int loc = GetUniformLocation(name);
    if (loc != -1)
        m_ShaderManager.SetUniformMatrix2fv(loc, &mat[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    int loc = GetUniformLocation(name);
    if (loc != -1)
        m_ShaderManager.SetUniformMatrix3fv(loc, &mat[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    setMat4(GetUniformLocation(name), mat);
}

void Shader::setMat4(int location, const glm::mat4& mat) const
{
    if (location != -1)
        m_ShaderManager.SetUniformMatrix4fv(location, &mat[0][0]);
}

void Shader::setVec3Array(const std::string& name, const glm::vec3* values, int count) const
{
    setVec3Array(GetUniformLocation(name), values, count);
}

void Shader::setVec3Array(int location, const glm::vec3* values, int count) const
{
    if (location != -1 && values != nullptr && count > 0)
        m_ShaderManager.SetUniform3fvArray(location, count, &values[0][0]);
}

void Shader::setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const
{
    setMat4Array(GetUniformLocation(name), matrices);
}

void Shader::setMat4Array(int location, const std::vector<glm::mat4>& matrices) const
{
    if (matrices.empty() || location == -1)
        return;
    int count = static_cast<int>(matrices.size());
    if (count > 128)
        count = 128;

    m_ShaderManager.SetUniformMatrix4fvArray(location, count, &matrices[0][0][0]);
}

void Shader::setCustomPorts(const ShaderPorts& ports) const
{
    int loc = GetUniformLocation("u_CustomPorts");
    if (loc != -1)
    {
        m_ShaderManager.SetUniform1fv(loc, 8, ports.data);
    }
}

int Shader::GetUniformLocation(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_UniformMutex);
    auto it = m_UniformLocations.find(name);
    if (it != m_UniformLocations.end())
        return it->second;

    int location = m_ShaderManager.GetUniformLocation(ID, name.c_str());
    m_UniformLocations[name] = location;
    return location;
}

void Shader::checkCompileErrors(unsigned int shader, std::string type)
{
    auto& sm = m_ShaderManager;
    if (type != "PROGRAM")
    {
        if (!sm.GetShaderCompileStatus(shader))
        {
            m_IsError = true;
            std::string infoLog = sm.GetShaderInfoLog(shader);
            LOGGER_ERROR("Shader") << "COMPILATION_ERROR in '" << m_Name << "' (type: " << type << "):\n"
                                   << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    else
    {
        if (!sm.GetProgramLinkStatus(shader))
        {
            m_IsError = true;
            std::string infoLog = sm.GetProgramInfoLog(shader);
            LOGGER_ERROR("Shader") << "LINKING_ERROR in '" << m_Name << "':\n"
                                   << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
}
