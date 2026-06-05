#pragma once

#include <core/logic/event_manager.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_resource_libraries.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <memory>
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
    int priority = 0;
    bool affectUI = false;
};

class PostProcessPipeline
{
public:
    PostProcessPipeline();
    ~PostProcessPipeline();

    void Initialize(IGraphicsContext& context, int width, int height, IShaderLibrary& shaderLib);
    void Shutdown();
    void Resize(int width, int height);

    void BeginCapture();
    void EndCapture();
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4& prevViewProj, const glm::mat4& currViewProj,
                           const glm::vec2& jitterOffset);

    void AddEffect(std::shared_ptr<Shader> shader, bool affectUI = false);
    void AddEffect(std::shared_ptr<Shader> shader, int priority, bool affectUI = false);
    void AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h, int priority = 0, bool affectUI = false);

    bool HasUIEffects() const;
    void RenderUIEffects();

    uint32_t GetDepthTexture() const
    {
        return m_DepthTexture ? m_DepthTexture->Get() : 0;
    }
    uint32_t GetCaptureFBO() const
    {
        return m_PingPong.fbo[0] ? m_PingPong.fbo[0]->Get() : 0;
    }
    uint32_t GetFinalColorTexture() const
    {
        return m_PingPong.color[m_PingPong.currentIndex] ? m_PingPong.color[m_PingPong.currentIndex]->Get() : 0;
    }

    void ClearEffects();

    float GetGamma() const;
    float GetExposure() const;

    int GetWidth() const
    {
        return m_Width;
    }
    int GetHeight() const
    {
        return m_Height;
    }

    void SetBloomEnabled(bool enable)
    {
        m_BloomEnabled = enable;
    }
    void SetBloomThreshold(float threshold)
    {
        m_BloomThreshold = threshold;
    }
    void SetBloomIntensity(float intensity)
    {
        m_BloomIntensity = intensity;
    }
    void SetBloomRadius(float radius)
    {
        m_BloomRadius = radius;
    }
    void SetHDREnabled(bool enabled)
    {
        m_HDREnabled = enabled;
    }
    void SetExposure(float exposure)
    {
        m_Exposure = exposure;
    }
    void SetGamma(float gamma)
    {
        m_Gamma = gamma;
    }
    void SetTonemappingMode(int mode)
    {
        m_TonemappingMode = mode;
    }

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0, m_Height = 0;

    struct PingPongBuffer
    {
        std::unique_ptr<GPUFramebuffer> fbo[2];
        std::unique_ptr<GPUTexture> color[2];
        int currentIndex = 0;

        GPUFramebuffer& CurrentFBO()
        {
            return *fbo[currentIndex];
        }
        GPUFramebuffer& PreviousFBO()
        {
            return *fbo[1 - currentIndex];
        }
        GPUTexture& CurrentColor()
        {
            return *color[currentIndex];
        }
        GPUTexture& PreviousColor()
        {
            return *color[1 - currentIndex];
        }
        void Swap()
        {
            currentIndex ^= 1;
        }
    } m_PingPong;

    std::unique_ptr<GPUTexture> m_DepthTexture;

    std::unique_ptr<GPUFramebuffer> m_HistoryFBO;
    std::unique_ptr<GPUTexture> m_HistoryTexture;

    std::shared_ptr<Shader> m_FXAAShader;
    std::shared_ptr<Shader> m_TAAShader;
    std::shared_ptr<Shader> m_BloomDownsampleShader;
    std::shared_ptr<Shader> m_BloomUpsampleShader;
    std::shared_ptr<Shader> m_HDRFinalShader;

    struct BloomMip
    {
        std::unique_ptr<GPUTexture> texture;
        int width, height;
    };
    std::vector<BloomMip> m_BloomMips;

    GpuHandle m_QuadVAO;
    GpuHandle m_QuadVBO;
    GpuHandle m_PartialVAO;
    GpuHandle m_PartialVBO;

    std::vector<PostProcessEffect> m_Effects;

    glm::vec4 m_ClearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    bool m_HDREnabled = false;
    bool m_BloomEnabled = false;
    float m_BloomThreshold = 1.0f;
    float m_BloomIntensity = 1.0f;
    float m_BloomRadius = 0.005f;
    float m_Exposure = 1.0f;
    float m_Gamma = 2.2f;
    int m_TonemappingMode = 1;
    EventSubscriptionList m_EventSubscriptions;

    void UpdateConfig();
    void InitQuad();
    void InitFramebuffers();
    void RenderBloom(uint32_t srcTexture);
    void RenderEffectsRange(int minPriority, int maxPriority, bool affectUI);
};
