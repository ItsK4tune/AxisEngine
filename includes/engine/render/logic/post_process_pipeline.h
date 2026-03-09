#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <render/logic/shader.h>
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

    void Init(IGraphicsContext& context, int width, int height, IShaderLibrary &shaderLib);
    void Shutdown();
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset);

    void AddEffect(std::shared_ptr<Shader> shader);
    void AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h);

    uint32_t GetDepthTexture() const { return m_DepthTexture ? m_DepthTexture->Get() : 0; }

    void ClearEffects();

    // Config setters
    void SetGamma(float gamma) { m_Gamma = gamma; }
    void SetExposure(float exposure) { m_Exposure = exposure; }
    void SetBloomIntensity(float intensity) { m_BloomIntensity = intensity; }
    void SetTonemappingMode(int mode) { m_TonemappingMode = mode; }
    void SetHDREnabled(bool enabled) { m_HDREnabled = enabled; }
    void SetBloomEnabled(bool enabled) { m_BloomEnabled = enabled; }
    void SetClearColor(float r, float g, float b, float a) { m_ClearColor[0]=r; m_ClearColor[1]=g; m_ClearColor[2]=b; m_ClearColor[3]=a; }

    float GetGamma() const { return m_Gamma; }
    float GetExposure() const { return m_Exposure; }

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

    GpuHandle m_QuadVAO;
    GpuHandle m_QuadVBO;

    std::vector<PostProcessEffect> m_Effects;

    // Config values
    float m_Gamma = 2.2f;
    float m_Exposure = 1.0f;
    float m_BloomIntensity = 1.0f;
    int m_TonemappingMode = 1;
    bool m_HDREnabled = false;
    bool m_BloomEnabled = false;
    float m_ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

    void InitQuad();
    void InitFramebuffers();
};
