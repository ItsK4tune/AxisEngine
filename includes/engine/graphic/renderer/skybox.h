#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include <graphic/core/shader.h>

class Skybox
{
public:
    Skybox();
    ~Skybox();

    void Draw(Shader& shader);
    void LoadCubemap(const std::vector<std::string>& faces);
    unsigned int GetTextureID() const { return m_TextureID; }

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_TextureID = 0;

    void Init();
};