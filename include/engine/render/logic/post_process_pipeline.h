#pragma once

#include <core/logic/event_manager.h>
#include <audio/interface/i_audio_capture_service.h>
#include <render/type/post_process_input.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_resource_libraries.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IGraphicsContext;
class ResourceManager;
class TransientBufferRing;


enum class AntiAliasingMode;

struct PostProcessEffect
{
    std::shared_ptr<Shader> shader;
    int x = 0, y = 0;
    int width = 0, height = 0;
    int priority = 0;
    bool affectUI = false;
    PostProcessInput inputs = PostProcessInput::Color;
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
    void ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4& prevViewProj, const glm::mat4& currViewProj);

    void AddEffect(std::shared_ptr<Shader> shader, bool affectUI = false,
                   PostProcessInput inputs = PostProcessInput::Color);
    void AddEffect(std::shared_ptr<Shader> shader, int priority, bool affectUI = false,
                   PostProcessInput inputs = PostProcessInput::Color);
    void AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h, int priority = 0, bool affectUI = false,
                   PostProcessInput inputs = PostProcessInput::Color);

    bool HasUIEffects() const;
    void RenderUIEffects();
    void ResetTemporalHistory();

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
        return m_PingPong.CurrentColorHandle();
    }
    uint32_t GetFinalFBO()
    {
        return m_PingPong.CurrentFBO().Get();
    }

    void ClearEffects();

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
    void SetTAAFeedback(float feedback)
    {
        m_TAAFeedback = glm::clamp(feedback, 0.0f, 0.999f);
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
        int writeIndex = 1;
        GPUFramebuffer* externalCurrentFBO = nullptr;
        GPUTexture* externalCurrentColor = nullptr;

        GPUFramebuffer& CurrentFBO()
        {
            return externalCurrentFBO ? *externalCurrentFBO : *fbo[currentIndex];
        }
        GPUFramebuffer& PreviousFBO()
        {
            return *fbo[writeIndex];
        }
        GPUTexture& CurrentColor()
        {
            return externalCurrentColor ? *externalCurrentColor : *color[currentIndex];
        }
        GPUTexture& PreviousColor()
        {
            return *color[writeIndex];
        }
        uint32_t CurrentColorHandle() const
        {
            if (externalCurrentColor)
                return externalCurrentColor->Get();
            return color[currentIndex] ? color[currentIndex]->Get() : 0;
        }
        void ResetToCapture()
        {
            currentIndex = 0;
            writeIndex = 1;
            externalCurrentFBO = nullptr;
            externalCurrentColor = nullptr;
        }
        void SetExternalCurrent(GPUFramebuffer& framebuffer, GPUTexture& texture)
        {
            externalCurrentFBO = &framebuffer;
            externalCurrentColor = &texture;
            writeIndex = 1;
        }
        void Swap()
        {
            currentIndex = writeIndex;
            writeIndex = 1 - currentIndex;
            externalCurrentFBO = nullptr;
            externalCurrentColor = nullptr;
        }
    } m_PingPong;

    std::unique_ptr<GPUTexture> m_DepthTexture;
    std::unique_ptr<TransientBufferRing> m_PulseUpload;
    size_t m_PulseBufferCapacity = 0;
    size_t m_PulseBufferOffset = 0;
    size_t m_PulseBufferSize = 0;
    bool m_PulseUploadPending = false;

    std::unique_ptr<GPUFramebuffer> m_HistoryFBO[2];
    std::unique_ptr<GPUTexture> m_HistoryTexture[2];
    int m_HistoryIndex = 0;

    std::shared_ptr<Shader> m_FXAAShader;
    std::shared_ptr<Shader> m_TAAShader;
    float m_TAAFeedback = 0.95f;
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

    std::vector<PostProcessEffect> m_Effects;
    bool m_EffectsDirty = false;
    bool m_HasUIEffects = false;

    struct FrameInputs
    {
        uint32_t depthTexture = 0;
        uint32_t normalTexture = 0;
        uint32_t worldPositionTexture = 0;
        glm::mat4 inverseViewProjection{1.0f};
        AudioCaptureSnapshot audio;
        size_t pulseCount = 0;
        bool hasCameraMatrices = false;
        bool prepared = false;
    } m_FrameInputs;

    glm::vec4 m_ClearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    bool m_HDREnabled = false;
    bool m_BloomEnabled = false;
    float m_BloomThreshold = 1.0f;
    float m_BloomIntensity = 1.0f;
    float m_BloomRadius = 0.005f;
    float m_Exposure = 1.0f;
    float m_Gamma = 2.2f;
    int m_TonemappingMode = 1;
    bool m_ResetTemporalHistory = true;
    EventSubscriptionList m_EventSubscriptions;

    void UpdateConfig();
    void InitQuad();
    void InitFramebuffers();
    void PrepareFrameInputs();
    void CommitPulseUpload();
    void RenderBloom(uint32_t srcTexture);
    void RenderEffectsRange(int minPriority, int maxPriority, bool affectUI);
};
