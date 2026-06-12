#include <render/rhi/rhi_graphics_context.h>
#include <core/logic/backend_factory_registry.h>
#include <core/logic/logger.h>
#include <render/rhi/render_backend_factory.h>
#include <stdexcept>

namespace
{
[[noreturn]] void ThrowLegacyManagerAccess(const char* managerName)
{
    throw std::runtime_error(std::string(managerName) +
                             " is part of the legacy OpenGL render pipeline and is not available on RHI contexts.");
}
}  // namespace

RhiGraphicsContext::RhiGraphicsContext(const AppConfig& config)
    : m_Config(config), m_ViewportWidth(config.width), m_ViewportHeight(config.height)
{
}

RhiGraphicsContext::~RhiGraphicsContext()
{
    Shutdown();
}

void RhiGraphicsContext::SetWindow(IWindow* window)
{
    m_Window = window;
    if (window)
    {
        m_ViewportWidth = window->GetWidth();
        m_ViewportHeight = window->GetHeight();
    }
}

bool RhiGraphicsContext::Initialize()
{
    m_Backend = rhi::CreateRenderBackend(m_Config.graphicsBackend);
    if (!m_Backend)
        return false;

    rhi::RenderBackendCreateInfo createInfo;
    createInfo.nativeWindow = m_Window ? m_Window->GetNativeWindow() : nullptr;
    createInfo.width = static_cast<uint32_t>(m_ViewportWidth > 0 ? m_ViewportWidth : m_Config.width);
    createInfo.height = static_cast<uint32_t>(m_ViewportHeight > 0 ? m_ViewportHeight : m_Config.height);
    createInfo.enableValidation = m_Config.logLevel == LogLevel::Debug;
    createInfo.applicationName = m_Config.title;
    createInfo.vsync = m_Config.vsync;

    if (!m_Backend->Initialize(createInfo))
    {
        LOGGER_ERROR("RhiGraphicsContext") << "Failed to initialize " << GetName() << " backend.";
        m_Backend.reset();
        return false;
    }

    m_FrameRenderer = std::make_unique<RhiFrameRenderer>(*m_Backend);
    m_SceneRenderer = std::make_unique<RhiSceneRenderer>(*m_Backend);
    LOGGER_INFO("RhiGraphicsContext") << "Initialized native RHI backend: " << m_Backend->GetName();
    return true;
}

void RhiGraphicsContext::Shutdown()
{
    m_SceneRenderer.reset();
    m_FrameRenderer.reset();
    if (m_Backend)
    {
        m_Backend->Shutdown();
        m_Backend.reset();
    }
    m_FrameActive = false;
}

bool RhiGraphicsContext::BeginFrame()
{
    if (!m_Backend)
        return false;
    if (!m_Backend->BeginFrame())
        return false;

    m_FrameActive = true;
    ClearBackBuffer();
    return true;
}

void RhiGraphicsContext::EndFrame()
{
    if (!m_Backend || !m_FrameActive)
        return;
    m_Backend->EndFrame();
    m_FrameActive = false;
}

bool RhiGraphicsContext::SupportsLegacyRenderPipeline() const
{
    return false;
}

bool RhiGraphicsContext::RenderNativeScene(Scene& scene, int width, int height, float alpha)
{
    if (!m_SceneRenderer || !m_FrameActive)
        return false;
    return m_SceneRenderer->Render(scene, width, height, alpha);
}

void RhiGraphicsContext::SetViewport(int, int, int width, int height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
    if (m_Backend && width > 0 && height > 0)
    {
        m_Backend->OnResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        if (m_SceneRenderer)
            m_SceneRenderer->OnResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }
}

void RhiGraphicsContext::SetDepthTest(bool enabled)
{
    (void)enabled;
}

void RhiGraphicsContext::SetCullFace(bool enabled)
{
    (void)enabled;
}

void RhiGraphicsContext::SetBlending(bool enabled)
{
    (void)enabled;
}

void RhiGraphicsContext::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    (void)src;
    (void)dst;
}

void RhiGraphicsContext::Clear(BufferBit flags)
{
    (void)flags;
    ClearBackBuffer();
}

IBufferManager& RhiGraphicsContext::GetBufferManager()
{
    ThrowLegacyManagerAccess("IBufferManager");
}

ITextureManager& RhiGraphicsContext::GetTextureManager()
{
    ThrowLegacyManagerAccess("ITextureManager");
}

IShaderManager& RhiGraphicsContext::GetShaderManager()
{
    ThrowLegacyManagerAccess("IShaderManager");
}

IRenderTargetManager& RhiGraphicsContext::GetRenderTargetManager()
{
    ThrowLegacyManagerAccess("IRenderTargetManager");
}

IRenderStateManager& RhiGraphicsContext::GetRenderStateManager()
{
    ThrowLegacyManagerAccess("IRenderStateManager");
}

IDrawContext& RhiGraphicsContext::GetDrawContext()
{
    ThrowLegacyManagerAccess("IDrawContext");
}

IQueryManager& RhiGraphicsContext::GetQueryManager()
{
    ThrowLegacyManagerAccess("IQueryManager");
}

rhi::IRenderBackend* RhiGraphicsContext::GetRenderBackend()
{
    return m_Backend.get();
}

std::string RhiGraphicsContext::GetName() const
{
    return m_Backend ? m_Backend->GetName() : "RHI";
}

void RhiGraphicsContext::ClearBackBuffer()
{
    if (!m_FrameRenderer)
        return;
    m_FrameRenderer->RenderSwapchainClear(m_Config.clearColor);
}

namespace axis::backend
{
#if AXIS_HAS_VULKAN_BACKEND
void RegisterVulkanBackendFactories()
{
    BackendFactoryRegistry::RegisterGraphics(
        GraphicsBackend::Vulkan, [](const AppConfig& config) { return std::make_unique<RhiGraphicsContext>(config); });
}
#endif

#if AXIS_HAS_DIRECTX_BACKEND
void RegisterDirectXBackendFactories()
{
    BackendFactoryRegistry::RegisterGraphics(
        GraphicsBackend::DirectX, [](const AppConfig& config) { return std::make_unique<RhiGraphicsContext>(config); });
}
#endif
}  // namespace axis::backend
