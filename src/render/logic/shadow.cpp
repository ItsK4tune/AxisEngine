#include <render/unit/shadow.h>
#include <render/interface/i_graphics_context.h>

#include <render/interface/i_draw_context.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <iostream>

IRenderTargetManager* Shadow::s_RenderTargetManager = nullptr;
ITextureManager* Shadow::s_TextureManager = nullptr;
IDrawContext* Shadow::s_DrawContext = nullptr;

void Shadow::SetManagers(IRenderTargetManager* rtm, ITextureManager* tm, IDrawContext* dc)
{
    s_RenderTargetManager = rtm;
    s_TextureManager = tm;
    s_DrawContext = dc;
}

Shadow::Shadow()
{

}

Shadow::~Shadow()
{
    Shutdown();
}

void Shadow::Shutdown()
{
    m_ShadowMapArray_Dir.reset();
    m_ShadowMapArray_Point.reset();
    m_ShadowMapArray_Spot.reset();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i) m_ShadowFBO_Dir[i].reset();
    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i) m_ShadowFBO_Point[i].reset();
    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i) m_ShadowFBO_Spot[i].reset();
}

void Shadow::Initialize(IGraphicsContext& context, unsigned int width, unsigned int height, bool manualControl, unsigned int pointWidth, unsigned int pointHeight)
{
    SHADOW_WIDTH = width;
    SHADOW_HEIGHT = height;
    SHADOW_POINT_WIDTH = pointWidth;
    SHADOW_POINT_HEIGHT = pointHeight;

    auto& tm = context.GetTextureManager();
    auto& rtm = context.GetRenderTargetManager();

    // Directional Shadow Array
    m_ShadowMapArray_Dir = std::make_unique<GPUTexture>(context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2DArray, m_ShadowMapArray_Dir->Get());
    tm.TexImage3D(TextureType::Texture2DArray, 0, InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, MAX_DIR_LIGHTS_SHADOW, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::WrapS, (int)TextureWrap::ClampToBorder);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::WrapT, (int)TextureWrap::ClampToBorder);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    tm.TexParameterfv(TextureType::Texture2DArray, TextureParameter::BorderColor, borderColor);

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i) {
        m_ShadowFBO_Dir[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[i]->Get());
        rtm.FramebufferTextureLayer(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, m_ShadowMapArray_Dir->Get(), 0, i);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
    }

    // Point Shadow Array
    m_ShadowMapArray_Point = std::make_unique<GPUTexture>(context, tm.GenTexture());
    tm.BindTexture(TextureType::TextureCubeMapArray, m_ShadowMapArray_Point->Get());
    tm.TexImage3D(TextureType::TextureCubeMapArray, 0, InternalFormat::DepthComponent24, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT, MAX_POINT_LIGHTS_SHADOW * 6, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
    tm.TexParameteri(TextureType::TextureCubeMapArray, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::TextureCubeMapArray, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::TextureCubeMapArray, TextureParameter::WrapS, (int)TextureWrap::ClampToEdge);
    tm.TexParameteri(TextureType::TextureCubeMapArray, TextureParameter::WrapT, (int)TextureWrap::ClampToEdge);
    tm.TexParameteri(TextureType::TextureCubeMapArray, TextureParameter::WrapR, (int)TextureWrap::ClampToEdge);

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i) {
        m_ShadowFBO_Point[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Point[i]->Get());
        rtm.FramebufferTexture(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, m_ShadowMapArray_Point->Get(), 0);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
    }

    // Spot Shadow Array
    m_ShadowMapArray_Spot = std::make_unique<GPUTexture>(context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2DArray, m_ShadowMapArray_Spot->Get());
    tm.TexImage3D(TextureType::Texture2DArray, 0, InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, MAX_SPOT_LIGHTS_SHADOW, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::WrapS, (int)TextureWrap::ClampToBorder);
    tm.TexParameteri(TextureType::Texture2DArray, TextureParameter::WrapT, (int)TextureWrap::ClampToBorder);
    tm.TexParameterfv(TextureType::Texture2DArray, TextureParameter::BorderColor, borderColor);

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i) {
        m_ShadowFBO_Spot[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[i]->Get());
        rtm.FramebufferTextureLayer(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, m_ShadowMapArray_Spot->Get(), 0, i);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
    }

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void Shadow::BindFBO_Dir(int index)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::BindFBO_Point(int index)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Point[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);
    }
}

void Shadow::BindFBO_Spot(int index)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::UnbindFBO()
{
    GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void Shadow::BindTexture_Dir(int index, int unit)
{
    // index is ignored as we bind the entire array
    auto& tm = GetTextureManager();
    tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
    tm.BindTexture(TextureType::Texture2DArray, m_ShadowMapArray_Dir->Get());
}

void Shadow::BindTexture_Point(int index, int unit)
{
    auto& tm = GetTextureManager();
    tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
    tm.BindTexture(TextureType::TextureCubeMapArray, m_ShadowMapArray_Point->Get());
}

void Shadow::BindTexture_Spot(int index, int unit)
{
    auto& tm = GetTextureManager();
    tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
    tm.BindTexture(TextureType::Texture2DArray, m_ShadowMapArray_Spot->Get());
}
