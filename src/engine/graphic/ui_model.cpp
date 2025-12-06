#include "engine/graphic/ui_model.h"

#include <iostream>

UIModel::UIModel(UIType type) : m_Type(type) {
    InitQuad();
}

UIModel::~UIModel() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void UIModel::SetTexture(unsigned int textureID) {
    m_TextureID = textureID;
    if(m_TextureID > 0) m_Type = UIType::Texture;
}

void UIModel::InitQuad() {
    float vertices[] = { 
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 
    
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void UIModel::Draw(Shader& shader, const glm::vec4& color) {
    if (m_Type == UIType::Transparent) return;

    shader.setVec4("spriteColor", color);

    shader.setInt("hasTexture", (m_Type == UIType::Texture)); 
    if (m_Type == UIType::Texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}