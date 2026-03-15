#include <render/logic/ui_model.h>
#include <render/type/graphics_types.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <iostream>
#include <core/logic/logger.h>

IBufferManager* UIModel::s_BufferManager = nullptr;
ITextureManager* UIModel::s_TextureManager = nullptr;
IDrawContext* UIModel::s_DrawContext = nullptr;

void UIModel::SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_TextureManager = &textureManager;
    s_DrawContext = &drawContext;
}

IBufferManager& UIModel::GetBufferManager()
{
    if (!s_BufferManager) {
        LOGGER_ERROR("UIModel") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in UIModel");
    }
    return *s_BufferManager;
}

ITextureManager& UIModel::GetTextureManager()
{
    if (!s_TextureManager) {
        LOGGER_ERROR("UIModel") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in UIModel");
    }
    return *s_TextureManager;
}

IDrawContext& UIModel::GetDrawContext()
{
    if (!s_DrawContext) {
        LOGGER_ERROR("UIModel") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in UIModel");
    }
    return *s_DrawContext;
}

UIModel::UIModel(UIType type) : m_Type(type)
{
    if (m_Type == UIType::Text)
    {
        InitDynamic();
    }
    else
    {
        InitQuad();
    }
}

UIModel::~UIModel()
{
    if (s_BufferManager)
    {
        if (VAO) s_BufferManager->DeleteVertexArrays(1, &VAO);
        if (VBO) s_BufferManager->DeleteBuffers(1, &VBO);
    }
}

void UIModel::SetTexture(unsigned int textureID)
{
    m_TextureID = textureID;
    if (m_TextureID > 0 && m_Type != UIType::Text)
        m_Type = UIType::Texture;
}

void UIModel::InitQuad()
{
    if (!s_BufferManager) return;
    auto& bm = GetBufferManager();

    float vertices[] = {
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f};

    VAO = bm.GenVertexArray();
    VBO = bm.GenBuffer();

    bm.BindVertexArray(VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, VBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(vertices), vertices, BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 4, DataType::Float, false, 4 * sizeof(float), (void *)0);

    bm.BindVertexArray(0);
}

void UIModel::InitDynamic()
{
    if (!s_BufferManager) return;
    auto& bm = GetBufferManager();

    VAO = bm.GenVertexArray();
    VBO = bm.GenBuffer();
    bm.BindVertexArray(VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, VBO);

    bm.BufferData(BufferType::ArrayBuffer, sizeof(float) * 6 * 4, NULL, BufferUsage::DynamicDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 4, DataType::Float, false, 4 * sizeof(float), 0);
    bm.BindVertexArray(0);
}

void UIModel::Draw(Shader &shader, const glm::vec4 &color, unsigned int textureID)
{
    if (m_Type == UIType::Transparent || !s_TextureManager || !s_DrawContext || !s_BufferManager)
        return;

    auto& tm = GetTextureManager();
    auto& dc = GetDrawContext();
    auto& bm = GetBufferManager();

    shader.setVec4("spriteColor", color);

    unsigned int texToBind = (textureID != 0) ? textureID : m_TextureID;
    bool hasTex = (texToBind != 0);

    shader.setBool("hasTexture", hasTex);
    if (hasTex)
    {
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, texToBind);
    }

    bm.BindVertexArray(VAO);
    dc.DrawArrays(Primitive::Triangles, 0, 6);
    bm.BindVertexArray(0);
}

void UIModel::DrawDynamic(Shader &shader, unsigned int textureID, const glm::vec4 &color, const std::vector<float> &vertices)
{
    if (!s_TextureManager || !s_DrawContext || !s_BufferManager) return;
    auto& tm = GetTextureManager();
    auto& dc = GetDrawContext();
    auto& bm = GetBufferManager();

    shader.setVec4("textColor", color);

    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, textureID);

    bm.BindVertexArray(VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, VBO);

    bm.BufferSubData(BufferType::ArrayBuffer, 0, vertices.size() * sizeof(float), vertices.data());

    dc.DrawArrays(Primitive::Triangles, 0, 6);

    bm.BindVertexArray(0);
    tm.BindTexture(TextureType::Texture2D, 0);
}