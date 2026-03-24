#pragma once
#include <ecs/interface/i_base_system.h>
#include <render/type/graphics_types.h>
#include <core/type/render_path.h>
#include <glm/glm.hpp>

class RenderQueue;
class StaticBatchManager;
class ShadowRenderer;
class MaterialRenderer;
class Shader;
struct Scene;
struct RenderItem;

enum class AntiAliasingMode
{
    NONE = 0,
    FXAA = 1,
    TAA = 2
};


class IRenderService : virtual public IBaseSystem
{
public:
    virtual ~IRenderService() = default;
    
    virtual int GetRenderedCount() const = 0;
    virtual uint32_t GetMainFBO() const = 0;
    virtual void SetMainFBO(uint32_t fbo) = 0;
    
    virtual StaticBatchManager& GetBatchManager() = 0;
    virtual RenderQueue& GetRenderQueueObj() = 0;
    
    virtual unsigned int GetWhiteTexture() const = 0;
    virtual unsigned int GetBlackTexture() const = 0;
    virtual unsigned int GetFlatNormalTexture() const = 0;
    
    virtual void ExecuteQueue(Scene& scene, const std::vector<RenderItem>& queue, bool isTransparentPass, ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer, Shader* overrideShader = nullptr) = 0;
    
    virtual void AddRenderedCount(int count) = 0;
    virtual AntiAliasingMode GetAntiAliasingMode() const = 0;
    virtual void SetAntiAliasingMode(AntiAliasingMode mode) = 0;
    virtual glm::mat4 GetPrevViewProj() const = 0;
    virtual glm::mat4 GetCurrViewProj() const = 0;
    virtual glm::vec2 GetJitterOffset() const = 0;
    
    virtual bool IsDebugNoTexture() const = 0;
    virtual void SetDebugNoTexture(bool enable) = 0;
    
    virtual bool IsWireframe() const = 0;
    virtual void SetWireframe(bool enable) = 0;
    
    virtual bool IsOcclusionCullingEnabled() const = 0;
    virtual void SetOcclusionCulling(bool enable) = 0;
    
    virtual void UpdateGlobalLightData(const GPUGlobalLightData& data) = 0;
    
    virtual RenderPath GetRenderPath() const = 0;
};
