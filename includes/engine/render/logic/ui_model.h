#pragma once

#include <glm/glm.hpp>
#include <render/logic/shader.h>
#include <vector>

class IBufferManager;
class IDrawContext;
class ITextureManager;

#define GLM_ENABLE_EXPERIMENTAL


enum class UIType {
    Color,
    Texture,
    Text,
    Transparent
};

class UIModel {
public:
    UIModel(UIType type = UIType::Color);
    ~UIModel();

    void SetTexture(unsigned int textureID);

    void Draw(Shader& shader, const glm::vec4& color, unsigned int textureID = 0);
    void DrawDynamic(Shader& shader, unsigned int textureID, const glm::vec4& color, const std::vector<float>& vertices);

    UIType GetType() const { return m_Type; }

    static void SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext);

private:
    unsigned int VAO = 0, VBO = 0;
    UIType m_Type;
    unsigned int m_TextureID = 0;

    void InitQuad();
    void InitDynamic();

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    static IBufferManager& GetBufferManager();
    static ITextureManager& GetTextureManager();
    static IDrawContext& GetDrawContext();
};