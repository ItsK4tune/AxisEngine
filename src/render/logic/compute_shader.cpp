#include <resource/unit/compute_shader.h>
#include <core/logic/logger.h>
#include <render/interface/i_shader_manager.h>
#include <fstream>
#include <sstream>
#include <utility>

ComputeShader::ComputeShader(IShaderManager& manager, std::string computePath)
    : m_ShaderManager(manager), m_Path(std::move(computePath))
{
    Reload();
}

bool ComputeShader::Reload()
{
    std::ifstream shaderFile;
    std::string computeCode;

    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        shaderFile.open(m_Path);
        std::stringstream stream;
        stream << shaderFile.rdbuf();
        computeCode = stream.str();
    }
    catch (const std::ifstream::failure& error)
    {
        LOGGER_ERROR("ComputeShader") << "Failed to read '" << m_Path << "': " << error.what();
        return false;
    }

    const unsigned int compute = m_ShaderManager.CreateShader(ShaderType::Compute);
    m_ShaderManager.ShaderSource(compute, computeCode.c_str());
    m_ShaderManager.CompileShader(compute);
    if (!CheckCompileErrors(compute, "COMPUTE"))
    {
        m_ShaderManager.DeleteShader(compute);
        return false;
    }

    const unsigned int program = m_ShaderManager.CreateProgram();
    m_ShaderManager.AttachShader(program, compute);
    m_ShaderManager.LinkProgram(program);
    m_ShaderManager.DeleteShader(compute);
    if (!CheckCompileErrors(program, "PROGRAM"))
    {
        m_ShaderManager.DeleteProgram(program);
        return false;
    }

    if (m_Program != 0)
        m_ShaderManager.DeleteProgram(m_Program);
    m_Program = program;
    return true;
}

ComputeShader::~ComputeShader()
{
    if (m_Program != 0)
        m_ShaderManager.DeleteProgram(m_Program);
}

void ComputeShader::use()
{
    if (m_Program != 0)
        m_ShaderManager.UseProgram(m_Program);
}

void ComputeShader::Dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ,
                             MemoryBarrierBit barrier)
{
    if (!IsValid() || groupsX == 0 || groupsY == 0 || groupsZ == 0)
        return;
    use();
    m_ShaderManager.DispatchCompute(groupsX, groupsY, groupsZ);
    m_ShaderManager.MemoryBarrier(barrier);
}

void ComputeShader::setBool(const std::string& name, bool value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform1i(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), (int)value);
}

void ComputeShader::setInt(const std::string& name, int value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform1i(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), value);
}

void ComputeShader::setFloat(const std::string& name, float value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform1f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), value);
}

void ComputeShader::setVec2(const std::string& name, const glm::vec2& value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform2f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), value.x, value.y);
}

void ComputeShader::setVec2(const std::string& name, float x, float y) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform2f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), x, y);
}

void ComputeShader::setVec3(const std::string& name, const glm::vec3& value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform3f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), value.x, value.y, value.z);
}

void ComputeShader::setVec3(const std::string& name, float x, float y, float z) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform3f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), x, y, z);
}

void ComputeShader::setVec4(const std::string& name, const glm::vec4& value) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform4f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), value.x, value.y, value.z,
                                 value.w);
}

void ComputeShader::setVec4(const std::string& name, float x, float y, float z, float w)
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniform4f(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), x, y, z, w);
}

void ComputeShader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniformMatrix2fv(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniformMatrix3fv(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    if (!IsValid())
        return;
    m_ShaderManager.SetUniformMatrix4fv(m_ShaderManager.GetUniformLocation(m_Program, name.c_str()), &mat[0][0]);
}

bool ComputeShader::CheckCompileErrors(unsigned int object, const char* type) const
{
    auto& sm = m_ShaderManager;

    bool success;
    if (std::string(type) != "PROGRAM")
    {
        success = sm.GetShaderCompileStatus(object);
        if (!success)
        {
            std::string infoLog = sm.GetShaderInfoLog(object);
            LOGGER_ERROR("ComputeShader") << "COMPILATION_ERROR of type: " << type << "\n"
                                          << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    else
    {
        success = sm.GetProgramLinkStatus(object);
        if (!success)
        {
            std::string infoLog = sm.GetProgramInfoLog(object);
            LOGGER_ERROR("ComputeShader") << "LINKING_ERROR of type: " << type << "\n"
                                          << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    return success;
}
