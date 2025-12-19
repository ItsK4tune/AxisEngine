#pragma once
#include <glad/glad.h>
#include <engine/graphic/shader.h>

class PostProcess
{
public:
    PostProcess();
    ~PostProcess();

    void Init(int width, int height);
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();

    void SetShader(Shader *shader) { m_Shader = shader; }
    bool IsActive() const { return m_Shader != nullptr; }

private:
    unsigned int m_FBO = 0;
    unsigned int m_TextureColorBuffer = 0;
    unsigned int m_RBO = 0;

    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;

    int m_Width = 0, m_Height = 0;
    Shader *m_Shader = nullptr;

    void InitQuad();
};