#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <graphic/core/shader.h>
#include <interface/graphic/graphics_types.h>
#include <memory>

enum class AntiAliasingMode;
class ResourceManager;
class IGraphicsContext;

struct PostProcessEffect
{
    std::shared_ptr<Shader> shader;
    int x = 0, y = 0;
    int width = 0, height = 0;
};

class PostProcessPipeline
{
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    void Init(IGraphicsContext& context, int width, int height, ResourceManager &res);
    void Shutdown();
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset);

    void AddEffect(std::shared_ptr<Shader> shader);
    void AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h);

    Graphics::GpuHandle GetDepthTexture() const { return m_DepthTexture; }

    void ClearEffects();

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0, m_Height = 0;

    Graphics::GpuHandle m_FBO[2];
    Graphics::GpuHandle m_ColorBuffers[2];
    Graphics::GpuHandle m_DepthTexture;

    Graphics::GpuHandle m_HistoryFBO;
    Graphics::GpuHandle m_HistoryTexture;

    std::shared_ptr<Shader> m_FXAAShader;
    std::shared_ptr<Shader> m_TAAShader;

    Graphics::GpuHandle m_QuadVAO;
    Graphics::GpuHandle m_QuadVBO;

    std::vector<PostProcessEffect> m_Effects;

    void InitQuad();
    void InitFramebuffers();
};
