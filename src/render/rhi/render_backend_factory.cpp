#include <render/rhi/render_backend_factory.h>
#include <core/logic/logger.h>

#if AXIS_HAS_OPENGL_BACKEND
#include <render/strategy/opengl/opengl_render_backend.h>
#endif

#if AXIS_HAS_VULKAN_BACKEND
#include <render/strategy/vulkan/vulkan_render_backend.h>
#endif

#if AXIS_HAS_DIRECTX_BACKEND
#include <render/strategy/directx/directx12_render_backend.h>
#endif

namespace rhi
{
std::unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsBackend backend)
{
    switch (backend)
    {
        case GraphicsBackend::OpenGL:
#if AXIS_HAS_OPENGL_BACKEND
            return std::make_unique<OpenGLRenderBackend>();
#else
            break;
#endif
        case GraphicsBackend::Vulkan:
#if AXIS_HAS_VULKAN_BACKEND
            return std::make_unique<VulkanRenderBackend>();
#else
            break;
#endif
        case GraphicsBackend::DirectX:
#if AXIS_HAS_DIRECTX_BACKEND
            return std::make_unique<DirectX12RenderBackend>();
#else
            break;
#endif
        case GraphicsBackend::Null:
            break;
    }

    LOGGER_ERROR("RenderBackendFactory") << "Requested graphics backend is not compiled into this build.";
    return nullptr;
}
}  // namespace rhi
