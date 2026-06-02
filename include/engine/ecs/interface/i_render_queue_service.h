#pragma once

#include <render/type/render_view_params.h>
#include <render/type/render_data.h>
#include <glm/glm.hpp>
#include <vector>

class RenderQueue;
class StaticBatchManager;
class ShadowRenderer;
class MaterialRenderer;
class Shader;
struct Scene;
struct RenderItem;
namespace entt
{
enum class entity : uint32_t;
}

struct RenderDrawCommand;

class IRenderQueueService
{
public:
    virtual ~IRenderQueueService() = default;

    virtual int GetRenderedCount() const = 0;
    virtual void AddRenderedCount(int count) = 0;

    virtual uint32_t GetMainFBO() const = 0;
    virtual void SetMainFBO(uint32_t fbo) = 0;

    virtual int GetLastWidth() const = 0;
    virtual int GetLastHeight() const = 0;

    virtual StaticBatchManager& GetBatchManager() = 0;
    virtual RenderQueue& GetRenderQueueObj() = 0;

    virtual void BuildRenderQueues(Scene& scene, float alpha, int width = 0, int height = 0) = 0;
    virtual void BuildRenderQueuesWithCamera(Scene& scene, const RenderViewParams& params) = 0;
    virtual void ExecuteQueue(const std::vector<RenderItem>& queue, RenderQueuePass pass,
                              ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer,
                              Shader* overrideShader = nullptr) = 0;

    virtual void SubmitCommand(const RenderDrawCommand& cmd) = 0;
    virtual void FlushCommands() = 0;
};
