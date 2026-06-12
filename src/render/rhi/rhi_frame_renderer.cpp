#include <render/rhi/rhi_frame_renderer.h>
#include <render/rhi/i_command_list.h>

RhiFrameRenderer::RhiFrameRenderer(rhi::IRenderBackend& backend) : m_Backend(backend)
{
}

bool RhiFrameRenderer::RenderSwapchainClear(const float (&clearColor)[4])
{
    auto& swapchain = m_Backend.GetSwapchain();
    rhi::ImageHandle backBuffer = swapchain.GetCurrentBackBuffer();
    if (!backBuffer)
        return false;

    rhi::Extent2D extent = swapchain.GetExtent();
    if (extent.width == 0 || extent.height == 0)
        return false;

    auto& device = m_Backend.GetDevice();
    auto& commandList = device.BeginCommandList(rhi::CommandQueueType::Graphics);

    rhi::RenderAttachmentDesc colorAttachment;
    colorAttachment.image = backBuffer;
    colorAttachment.format = swapchain.GetBackBufferFormat();
    colorAttachment.loadOp = rhi::LoadOp::Clear;
    colorAttachment.storeOp = rhi::StoreOp::Store;
    colorAttachment.clearColor = {clearColor[0], clearColor[1], clearColor[2], clearColor[3]};

    rhi::RenderPassBeginInfo beginInfo;
    beginInfo.colorAttachments.push_back(colorAttachment);
    beginInfo.renderArea = {0, 0, extent.width, extent.height};

    commandList.BeginRendering(beginInfo);
    commandList.EndRendering();
    device.Submit(commandList);
    return true;
}
