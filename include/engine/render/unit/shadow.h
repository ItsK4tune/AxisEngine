#pragma once

#include <render/type/graphics_types.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IDrawContext;
class IGraphicsContext;
class IRenderTargetManager;
class ITextureManager;


class Shadow
{
public:
    Shadow();
    ~Shadow();

    void Initialize(IGraphicsContext& context, unsigned int width = 2048, unsigned int height = 2048,
                    bool manualControl = false, unsigned int pointWidth = 1024, unsigned int pointHeight = 1024);
    void Shutdown();

    void BindFBO_Dir(int index);
    void BindFBO_Point(int index);
    void BindFBO_Spot(int index);
    void UnbindFBO();

    void BindTexture_Dir(int index, int unit);
    void BindTexture_Point(int index, int unit);
    void BindTexture_Spot(int index, int unit);

    unsigned int GetShadowWidth() const
    {
        return SHADOW_WIDTH;
    }
    unsigned int GetShadowHeight() const
    {
        return SHADOW_HEIGHT;
    }
    unsigned int GetShadowPointWidth() const
    {
        return SHADOW_POINT_WIDTH;
    }
    unsigned int GetShadowPointHeight() const
    {
        return SHADOW_POINT_HEIGHT;
    }

    Shader* GetShaderDir()
    {
        return m_ShadowShaderDir;
    }
    Shader* GetShaderPoint()
    {
        return m_ShadowShaderPoint;
    }
    Shader* GetShaderSpot()
    {
        return m_ShadowShaderSpot;
    }

    void SetShaderDir(Shader* shader)
    {
        m_ShadowShaderDir = shader;
    }
    void SetShaderPoint(Shader* shader)
    {
        m_ShadowShaderPoint = shader;
    }
    void SetShaderSpot(Shader* shader)
    {
        m_ShadowShaderSpot = shader;
    }

    static void SetManagers(IRenderTargetManager* rtm, ITextureManager* tm, IDrawContext* dc);
    static IRenderTargetManager& GetRenderTargetManager()
    {
        return *s_RenderTargetManager;
    }
    static ITextureManager& GetTextureManager()
    {
        return *s_TextureManager;
    }
    static IDrawContext& GetDrawContext()
    {
        return *s_DrawContext;
    }

    static const int MAX_DIR_LIGHTS_SHADOW = 16;
    static const int MAX_POINT_LIGHTS_SHADOW = 16;
    static const int MAX_SPOT_LIGHTS_SHADOW = 16;

private:
    unsigned int SHADOW_WIDTH, SHADOW_HEIGHT;
    unsigned int SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT;

    std::unique_ptr<GPUFramebuffer> m_ShadowFBO_Dir[MAX_DIR_LIGHTS_SHADOW];
    std::unique_ptr<GPUTexture> m_ShadowMapArray_Dir;

    std::unique_ptr<GPUFramebuffer> m_ShadowFBO_Point[MAX_POINT_LIGHTS_SHADOW];
    std::unique_ptr<GPUTexture> m_ShadowMapArray_Point;

    std::unique_ptr<GPUFramebuffer> m_ShadowFBO_Spot[MAX_SPOT_LIGHTS_SHADOW];
    std::unique_ptr<GPUTexture> m_ShadowMapArray_Spot;

    Shader* m_ShadowShaderDir = nullptr;
    Shader* m_ShadowShaderPoint = nullptr;
    Shader* m_ShadowShaderSpot = nullptr;

    static IRenderTargetManager* s_RenderTargetManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;
};
