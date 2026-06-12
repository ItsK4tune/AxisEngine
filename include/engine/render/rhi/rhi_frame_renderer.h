#pragma once

#include <render/rhi/i_render_backend.h>

class RhiFrameRenderer
{
public:
    explicit RhiFrameRenderer(rhi::IRenderBackend& backend);

    bool RenderSwapchainClear(const float (&clearColor)[4]);

private:
    rhi::IRenderBackend& m_Backend;
};
