#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <resource/unit/shader.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_resource_libraries.h>
#include <vector>

class IGraphicsContext;
class ResourceManager;

#define GLM_ENABLE_EXPERIMENTAL


enum class AntiAliasingMode;

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

    void Initialize(IGraphicsContext& context, int width, int height, IShaderLibrary &shaderLib);
    void Shutdown();
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset);

    void AddEffect(std::shared_ptr<Shader> shader);
    void AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h);

    uint32_t GetDepthTexture() const { return m_DepthTexture ? m_DepthTexture->Get() : 0; }
    uint32_t GetCaptureFBO() const { return m_PingPong.fbo[0] ? m_PingPong.fbo[0]->Get() : 0; }

    void ClearEffects();

    // Config setters
    // Config updates
    void UpdateConfig();

    float GetGamma() const;
    float GetExposure() const;

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0, m_Height = 0;

    struct PingPongBuffer {
        std::unique_ptr<GPUFramebuffer> fbo[2];
        std::unique_ptr<GPUTexture> color[2];
        int currentIndex = 0;

        GPUFramebuffer& CurrentFBO() { return *fbo[currentIndex]; }
        GPUFramebuffer& PreviousFBO() { return *fbo[1 - currentIndex]; }
        GPUTexture& CurrentColor() { return *color[currentIndex]; }
        GPUTexture& PreviousColor() { return *color[1 - currentIndex]; }
        void Swap() { currentIndex ^= 1; }
    } m_PingPong;

    std::unique_ptr<GPUTexture> m_DepthTexture;

    std::unique_ptr<GPUFramebuffer> m_HistoryFBO;
    std::unique_ptr<GPUTexture> m_HistoryTexture;

    std::shared_ptr<Shader> m_FXAAShader;
    std::shared_ptr<Shader> m_TAAShader;
    std::shared_ptr<Shader> m_BloomDownsampleShader;
    std::shared_ptr<Shader> m_BloomUpsampleShader;
    std::shared_ptr<Shader> m_HDRFinalShader;

    struct BloomMip {
        std::unique_ptr<GPUTexture> texture;
        int width, height;
    };
    std::vector<BloomMip> m_BloomMips;

    GpuHandle m_QuadVAO;
    GpuHandle m_QuadVBO;

    std::vector<PostProcessEffect> m_Effects;

    glm::vec4 m_ClearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    bool m_BloomEnabled = true;
    float m_BloomThreshold = 1.0f;
    float m_BloomIntensity = 1.0f;
    float m_BloomRadius = 0.005f;
    float m_Exposure = 1.0f;
    float m_Gamma = 2.2f;
    int m_TonemappingMode = 1;

    uint32_t m_ConfigSubId = 0;

    void InitQuad();
    void InitFramebuffers();
    void RenderBloom(uint32_t srcTexture);
};
