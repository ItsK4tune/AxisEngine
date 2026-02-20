#include <graphic/core/compute_shader.h>
#include <fstream>
#include <sstream>
#include <utils/logger.h>
#include <interface/graphic/i_shader_manager.h>

static constexpr unsigned int COMPUTE_SHADER = 0x91B9;
static constexpr unsigned int FALSE = 0;

IShaderManager* ComputeShader::s_ShaderManager = nullptr;

void ComputeShader::SetShaderManager(IShaderManager& shaderManager)
{
    s_ShaderManager = &shaderManager;
}

IShaderManager& ComputeShader::GetShaderManager()
{
    if (!s_ShaderManager) {
        LOGGER_ERROR("ComputeShader") << "ShaderManager not set!";
        throw std::runtime_error("ShaderManager not set in ComputeShader");
    }
    return *s_ShaderManager;
}

ComputeShader::ComputeShader(const char *computePath)
{
    if (!s_ShaderManager) return;

    std::string computeCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        cShaderFile.open(computePath);

        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();
        cShaderFile.close();
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
        LOGGER_ERROR("ComputeShader") << "FILE_NOT_SUCCESSFULLY_READ: " << e.what();
    }
    const char *cShaderCode = computeCode.c_str();

    unsigned int compute;
    auto& sm = GetShaderManager();

    compute = sm.CreateShader(Graphics::ShaderType::Compute);
    sm.ShaderSource(compute, cShaderCode);
    sm.CompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");

    ID = sm.CreateProgram();
    sm.AttachShader(ID, compute);
    sm.LinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    sm.DeleteShader(compute);
}

ComputeShader::~ComputeShader()
{
    if (s_ShaderManager && ID != 0)
    {
        GetShaderManager().DeleteProgram(ID);
    }
}

void ComputeShader::use()
{
    if (s_ShaderManager) GetShaderManager().UseProgram(ID);
}

void ComputeShader::setBool(const std::string &name, bool value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform1i(GetShaderManager().GetUniformLocation(ID, name.c_str()), (int)value);
}

void ComputeShader::setInt(const std::string &name, int value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform1i(GetShaderManager().GetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setFloat(const std::string &name, float value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform1f(GetShaderManager().GetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setVec2(const std::string &name, const glm::vec2 &value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform2f(GetShaderManager().GetUniformLocation(ID, name.c_str()), value.x, value.y);
}

void ComputeShader::setVec2(const std::string &name, float x, float y) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform2f(GetShaderManager().GetUniformLocation(ID, name.c_str()), x, y);
}

void ComputeShader::setVec3(const std::string &name, const glm::vec3 &value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform3f(GetShaderManager().GetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
}

void ComputeShader::setVec3(const std::string &name, float x, float y, float z) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform3f(GetShaderManager().GetUniformLocation(ID, name.c_str()), x, y, z);
}

void ComputeShader::setVec4(const std::string &name, const glm::vec4 &value) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform4f(GetShaderManager().GetUniformLocation(ID, name.c_str()), value.x, value.y, value.z, value.w);
}

void ComputeShader::setVec4(const std::string &name, float x, float y, float z, float w)
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniform4f(GetShaderManager().GetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void ComputeShader::setMat2(const std::string &name, const glm::mat2 &mat) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniformMatrix2fv(GetShaderManager().GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat3(const std::string &name, const glm::mat3 &mat) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniformMatrix3fv(GetShaderManager().GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    if (!s_ShaderManager) return;
    GetShaderManager().SetUniformMatrix4fv(GetShaderManager().GetUniformLocation(ID, name.c_str()), &mat[0][0]);
}

void ComputeShader::checkCompileErrors(unsigned int shader, std::string type)
{
    if (!s_ShaderManager) return;
    auto& sm = GetShaderManager();

    bool success;
    if (type != "PROGRAM")
    {
        success = sm.GetShaderCompileStatus(shader);
        if (!success)
        {
            std::string infoLog = sm.GetShaderInfoLog(shader);
            LOGGER_ERROR("ComputeShader") << "COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
    else
    {
        success = sm.GetProgramLinkStatus(shader);
        if (!success)
        {
            std::string infoLog = sm.GetProgramInfoLog(shader);
            LOGGER_ERROR("ComputeShader") << "LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- ";
        }
    }
}
