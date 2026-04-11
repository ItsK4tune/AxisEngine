#pragma once
#include <render/logic/material_renderer.h>
#include <render/interface/i_graphics_context.h>
#include <render/type/graphics_types.h>

class RenderCore {
public:
    RenderCore() = default;
    ~RenderCore() = default;

    void Initialize(class IGraphicsContext* context);
    void Shutdown();

    uint32_t GetWhiteTexture() const { return m_WhiteTextureID; }
    uint32_t GetBlackTexture() const { return m_BlackTextureID; }
    uint32_t GetFlatNormalTexture() const { return m_FlatNormalTextureID; }

    uint32_t GetQuadVAO() const { return m_QuadVAO; }
    uint32_t GetQuadEBO() const { return m_QuadEBO; }
    
    uint32_t GetCubeVAO() const { return m_CubeVAO; }
    uint32_t GetCubeEBO() const { return m_CubeEBO; }

    MaterialRenderer& GetMaterialRenderer() { return m_MaterialRenderer; }

private:
    uint32_t m_WhiteTextureID = 0;
    uint32_t m_BlackTextureID = 0;
    uint32_t m_FlatNormalTextureID = 0;

    uint32_t m_QuadVAO = 0;
    uint32_t m_QuadVBO = 0;
    uint32_t m_QuadEBO = 0;

    uint32_t m_CubeVAO = 0;
    uint32_t m_CubeVBO = 0;
    uint32_t m_CubeEBO = 0;

    MaterialRenderer m_MaterialRenderer;
    IGraphicsContext* m_Context = nullptr;

    void InitQuad();
    void InitCube();
};
