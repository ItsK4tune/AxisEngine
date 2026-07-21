#include <render/strategy/opengl/opengl_context.h>
#include <core/logic/logger.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

bool OpenGLContext::Initialize()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOGGER_ERROR("OpenGLContext") << "Failed to initialize GLAD";
        return false;
    }

    LOGGER_INFO("OpenGLContext") << "OpenGL initialized: " << glGetString(GL_VERSION);

    m_DrawContext.SetRenderStateManager(&m_RenderStateManager);
    m_Initialized = true;

    return true;
}

void OpenGLContext::Shutdown()
{
    if (!m_Initialized)
        return;
    m_DrawContext.SetRenderStateManager(nullptr);
    m_Initialized = false;
}

void OpenGLContext::SetViewport(int x, int y, int width, int height)
{
    m_RenderStateManager.SetViewport(x, y, width, height);
}

void OpenGLContext::SetDepthTest(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::DepthTest);
    else
        m_RenderStateManager.Disable(ServerCapability::DepthTest);
}

void OpenGLContext::SetCullFace(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::CullFace);
    else
        m_RenderStateManager.Disable(ServerCapability::CullFace);
}

void OpenGLContext::SetBlending(bool enable)
{
    if (enable)
        m_RenderStateManager.Enable(ServerCapability::Blend);
    else
        m_RenderStateManager.Disable(ServerCapability::Blend);
}

void OpenGLContext::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    m_RenderStateManager.SetBlendFunc(src, dst);
}

#include <render/strategy/opengl/opengl_translator.h>

void OpenGLContext::Clear(BufferBit flags)
{
    glClear(GLTranslator::ToGL(flags));
}

void OpenGLContext::InvalidateStateCache()
{
    m_BufferManager.InvalidateCache();
    m_TextureManager.InvalidateCache();
    m_ShaderManager.InvalidateCache();
    m_RenderTargetManager.InvalidateCache();
    m_RenderStateManager.InvalidateCache();
}

void OpenGLContext::SetStateCacheEnabled(bool enabled)
{
    m_BufferManager.SetCacheEnabled(enabled);
    m_TextureManager.SetCacheEnabled(enabled);
    m_ShaderManager.SetCacheEnabled(enabled);
    m_RenderTargetManager.SetCacheEnabled(enabled);
    m_RenderStateManager.SetCacheEnabled(enabled);
}

bool OpenGLContext::TryGetMemoryBudget(uint64_t& usedBytes, uint64_t& totalBytes) const
{
    if (!m_Initialized || !glGetIntegerv || !glGetStringi)
        return false;

    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    bool hasMemoryInfo = false;
    for (GLint index = 0; index < extensionCount; ++index)
    {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, index));
        if (extension && std::strcmp(extension, "GL_NVX_gpu_memory_info") == 0)
        {
            hasMemoryInfo = true;
            break;
        }
    }
    if (!hasMemoryInfo)
        return false;

    GLint dedicatedKb = 0;
    GLint reportedTotalKb = 0;
    GLint availableKb = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedKb);
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &reportedTotalKb);
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableKb);
    const GLint effectiveTotalKb = dedicatedKb > 0 ? dedicatedKb : reportedTotalKb;
    if (effectiveTotalKb <= 0 || availableKb < 0)
        return false;

    totalBytes = static_cast<uint64_t>(effectiveTotalKb) * 1024ULL;
    usedBytes = static_cast<uint64_t>(std::max(effectiveTotalKb - availableKb, 0)) * 1024ULL;
    return true;
}
