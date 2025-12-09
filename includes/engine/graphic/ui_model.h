#pragma once

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include <engine/graphic/shader.h>

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

private:
    unsigned int VAO = 0, VBO = 0;
    UIType m_Type;
    unsigned int m_TextureID = 0;

    void InitQuad();
    void InitDynamic();
};