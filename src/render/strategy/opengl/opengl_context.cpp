#include <render/strategy/opengl/opengl_context.h>
#include <core/logic/backend_factory_registry.h>
#include <core/logic/logger.h>
#include <render/rhi/rhi_graphics_context.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

namespace
{
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif
#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#endif
#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

bool HasOpenGLExtension(const char* name)
{
    if (!glGetIntegerv || !glGetStringi || !name)
        return false;

    GLint extensionCount = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    for (GLint i = 0; i < extensionCount; ++i)
    {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        if (extension && std::strcmp(extension, name) == 0)
            return true;
    }
    return false;
}
}  // namespace

bool OpenGLContext::Initialize()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOGGER_ERROR("OpenGLContext") << "Failed to initialize GLAD";
        return false;
    }

    LOGGER_INFO("OpenGLContext") << "OpenGL initialized: " << glGetString(GL_VERSION);

    m_DrawContext.SetRenderStateManager(&m_RenderStateManager);
    m_RenderBackend.Initialize({});

    return true;
}

void OpenGLContext::Shutdown()
{
    m_RenderBackend.Shutdown();
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

bool OpenGLContext::TryGetVramUsage(uint64_t& usedBytes, uint64_t& totalBytes) const
{
    if (!glGetIntegerv || !HasOpenGLExtension("GL_NVX_gpu_memory_info"))
        return false;

    GLint dedicatedKb = 0;
    GLint totalKb = 0;
    GLint availableKb = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedKb);
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalKb);
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableKb);

    const GLint effectiveTotalKb = dedicatedKb > 0 ? dedicatedKb : totalKb;
    if (effectiveTotalKb <= 0 || availableKb < 0)
        return false;

    totalBytes = static_cast<uint64_t>(effectiveTotalKb) * 1024ULL;
    usedBytes = static_cast<uint64_t>((std::max)(effectiveTotalKb - availableKb, 0)) * 1024ULL;
    return true;
}

std::string OpenGLContext::GetDeviceName() const
{
    const auto* renderer = glGetString(GL_RENDERER);
    return renderer ? reinterpret_cast<const char*>(renderer) : GetName();
}

namespace axis::backend
{
void RegisterOpenGLBackendFactories()
{
    BackendFactoryRegistry::RegisterGraphics(
        GraphicsBackend::OpenGL, [](const AppConfig& config) { return std::make_unique<RhiGraphicsContext>(config); });
}
}  // namespace axis::backend
