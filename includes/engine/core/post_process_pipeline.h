#pragma once
#include <glad/glad.h>
#include <vector>
#include <graphic/shader.h>

struct PostProcessEffect {
    Shader* shader;
    int x = 0, y = 0; 
    int width = 0, height = 0; 
};

class PostProcessPipeline
{
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    void Init(int width, int height);
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();

    void AddEffect(Shader* shader);
    void AddEffect(Shader* shader, int x, int y, int w, int h);
    
    void ClearEffects();

private:
    int m_Width = 0, m_Height = 0;
    
    // Ping-Pong buffers
    unsigned int m_FBO[2];
    unsigned int m_ColorBuffers[2];
    unsigned int m_RBO; // Depth buffer chỉ cần cho FBO[0] (Scene render)

    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;

    std::vector<PostProcessEffect> m_Effects;

    void InitQuad();
    void InitFramebuffers();
};