#pragma once

#include <core/type/graphics_backend.h>
#include <render/rhi/i_render_backend.h>
#include <memory>

namespace rhi
{
std::unique_ptr<IRenderBackend> CreateRenderBackend(GraphicsBackend backend);
}  // namespace rhi
