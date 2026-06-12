#pragma once

#include <render/rhi/rhi_types.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Scene;

namespace rhi
{
class IRenderBackend;
}

class RhiSceneRenderer
{
public:
    explicit RhiSceneRenderer(rhi::IRenderBackend& backend);
    ~RhiSceneRenderer();

    bool Render(Scene& scene, int width, int height, float alpha);
    void OnResize(uint32_t width, uint32_t height);
    void Shutdown();

private:
    struct PushConstants
    {
        glm::mat4 mvp;
        glm::vec4 color;
    };

    bool EnsureResources(uint32_t width, uint32_t height);
    bool EnsureShaders();
    rhi::PipelineHandle GetOrCreatePipeline(uint32_t vertexStride);
    rhi::PipelineHandle GetOrCreateLitPipeline(uint32_t vertexStride);
    rhi::PipelineHandle GetOrCreateDecalPipeline(uint32_t vertexStride);
    rhi::PipelineHandle GetOrCreateSkyboxPipeline();
    void DestroyPipelines();

    std::vector<uint8_t> LoadShaderFile(const std::string& relativePath) const;

    rhi::IRenderBackend& m_Backend;
    rhi::ImageHandle m_DepthImage;
    uint32_t m_DepthWidth = 0;
    uint32_t m_DepthHeight = 0;

    rhi::ShaderModuleHandle m_VertexShader;
    rhi::ShaderModuleHandle m_FragmentShader;
    std::unordered_map<uint32_t, rhi::PipelineHandle> m_PipelinesByStride;

    rhi::ShaderModuleHandle m_SkyboxVS;
    rhi::ShaderModuleHandle m_SkyboxPS;
    rhi::ShaderModuleHandle m_LitVS;
    rhi::ShaderModuleHandle m_LitPS;
    rhi::ShaderModuleHandle m_DecalVS;
    rhi::ShaderModuleHandle m_DecalPS;

    rhi::BufferHandle m_SkyboxVbo;
    rhi::BufferHandle m_DecalQuadVbo;

    std::unordered_map<uint32_t, rhi::PipelineHandle> m_LitPipelinesByStride;
    std::unordered_map<uint32_t, rhi::PipelineHandle> m_DecalPipelinesByStride;
    rhi::PipelineHandle m_SkyboxPipeline;

    bool m_ShaderLoadFailed = false;
    bool m_FirstFrameLogged = false;
};
