#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <graphics/core/shader.h>
#include <vector>

class IBufferManager;
class ITextureManager;
class IDrawContext;

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

    void Draw(Shader& shader, const glm::vec4& color);
    void DrawDynamic(Shader& shader, unsigned int textureID, const glm::vec3& color, const std::vector<float>& vertices);

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
