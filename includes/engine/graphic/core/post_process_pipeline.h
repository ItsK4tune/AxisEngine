#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <graphic/core/shader.h>

enum class AntiAliasingMode;
class ResourceManager;

struct PostProcessEffect
{
    Shader *shader;
    int x = 0, y = 0;
    int width = 0, height = 0;
};

class PostProcessPipeline
{
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    void Init(int width, int height, ResourceManager &res);
    void Shutdown();
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset);

    void AddEffect(Shader *shader);
    void AddEffect(Shader *shader, int x, int y, int w, int h);
    
    unsigned int GetDepthTexture() const { return m_DepthTexture; }

    void ClearEffects();

private:
    int m_Width = 0, m_Height = 0;

    unsigned int m_FBO[2];
    unsigned int m_ColorBuffers[2];
    unsigned int m_DepthTexture = 0;
    
    unsigned int m_HistoryFBO = 0;
    unsigned int m_HistoryTexture = 0;
    
    Shader *m_FXAAShader = nullptr;
    Shader *m_TAAShader = nullptr;

    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;

    std::vector<PostProcessEffect> m_Effects;

    void InitQuad();
    void InitFramebuffers();
};