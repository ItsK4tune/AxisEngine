#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <rendering/core/shader.h>
#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>
#include <rendering/core/gpu_resources.h>
#include <resource/i_resource_libraries.h>

#include <memory>
#include <vector>

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

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0, m_Height = 0;

    struct PingPongBuffer {
        std::unique_ptr<Graphics::GPUFramebuffer> fbo[2];
        std::unique_ptr<Graphics::GPUTexture> color[2];
        int currentIndex = 0;

        Graphics::GPUFramebuffer& CurrentFBO() { return *fbo[currentIndex]; }
        Graphics::GPUFramebuffer& PreviousFBO() { return *fbo[1 - currentIndex]; }
        Graphics::GPUTexture& CurrentColor() { return *color[currentIndex]; }
        Graphics::GPUTexture& PreviousColor() { return *color[1 - currentIndex]; }
        void Swap() { currentIndex ^= 1; }
    } m_PingPong;

    std::unique_ptr<Graphics::GPUTexture> m_DepthTexture;

    std::unique_ptr<Graphics::GPUFramebuffer> m_HistoryFBO;
    std::unique_ptr<Graphics::GPUTexture> m_HistoryTexture;

    std::shared_ptr<Shader> m_FXAAShader;
    std::shared_ptr<Shader> m_TAAShader;

    Graphics::GpuHandle m_QuadVAO;
    Graphics::GpuHandle m_QuadVBO;

    std::vector<PostProcessEffect> m_Effects;

    void InitQuad();
    void InitFramebuffers();
};
