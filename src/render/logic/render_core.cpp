#include <render/logic/render_core.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <core/logic/logger.h>

void RenderCore::Initialize(IGraphicsContext& context)
{
    m_Context = &context;
    auto& tm = context.GetTextureManager();

    // 1. Textures
    m_WhiteTextureID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
    unsigned char white[] = { 255, 255, 255, 255 };
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, white);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

    m_BlackTextureID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_BlackTextureID);
    unsigned char black[] = { 0, 0, 0, 255 };
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, black);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

    m_FlatNormalTextureID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_FlatNormalTextureID);
    unsigned char flatNormal[] = { 128, 128, 255, 255 };
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, flatNormal);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Linear);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Linear);

    // 2. Material Renderer
    m_MaterialRenderer.Initialize(context, m_WhiteTextureID, m_BlackTextureID, m_FlatNormalTextureID);

    // 3. Meshes
    InitQuad();
    InitCube();

    LOGGER_INFO("RenderCore") << "Initialized RenderCore features";
}

void RenderCore::Shutdown()
{
    if (!m_Context) return;
    auto& bm = m_Context->GetBufferManager();
    auto& tm = m_Context->GetTextureManager();

    if (m_QuadVAO) bm.DeleteVertexArray(m_QuadVAO);
    if (m_QuadVBO) bm.DeleteBuffer(m_QuadVBO);
    if (m_QuadEBO) bm.DeleteBuffer(m_QuadEBO);
    if (m_CubeVAO) bm.DeleteVertexArray(m_CubeVAO);
    if (m_CubeVBO) bm.DeleteBuffer(m_CubeVBO);
    if (m_CubeEBO) bm.DeleteBuffer(m_CubeEBO);

    tm.DeleteTexture(m_WhiteTextureID);
    tm.DeleteTexture(m_BlackTextureID);
    tm.DeleteTexture(m_FlatNormalTextureID);
}

void RenderCore::InitQuad()
{
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    auto& bm = m_Context->GetBufferManager();
    m_QuadVAO = bm.CreateVertexArray();
    m_QuadVBO = bm.CreateBuffer();

    bm.BindVertexArray(m_QuadVAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_QuadVBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, BufferUsage::StaticDraw);
    
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    unsigned int indices[] = { 0, 1, 3, 0, 3, 2 };
    m_QuadEBO = bm.CreateBuffer();
    bm.BindBuffer(BufferType::ElementArrayBuffer, m_QuadEBO);
    bm.BufferData(BufferType::ElementArrayBuffer, sizeof(indices), indices, BufferUsage::StaticDraw);
}

void RenderCore::InitCube()
{
    float vertices[] = {
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
    };
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1,
        7, 6, 5, 5, 4, 7, 4, 0, 3, 3, 7, 4,
        4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3
    };
    auto& bm = m_Context->GetBufferManager();
    m_CubeVAO = bm.CreateVertexArray();
    m_CubeVBO = bm.CreateBuffer();
    m_CubeEBO = bm.CreateBuffer();
    
    bm.BindVertexArray(m_CubeVAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_CubeVBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(vertices), vertices, BufferUsage::StaticDraw);
    bm.BindBuffer(BufferType::ElementArrayBuffer, m_CubeEBO);
    bm.BufferData(BufferType::ElementArrayBuffer, sizeof(indices), indices, BufferUsage::StaticDraw);
    
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 3 * sizeof(float), (void*)0);
}
