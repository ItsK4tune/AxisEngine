#pragma once

#include <core/type/spatial_culling_mode.h>
#include <render/type/graphics_types.h>
#include <cstdint>
#include <string>
#include <vector>

struct FrameDebugDrawCall
{
    uint64_t index = 0;
    uint32_t entityId = 0;
    uint32_t shaderId = 0;
    uint32_t vertexArray = 0;
    uint32_t elementCount = 0;
    int layer = 0;
    std::string pass;
    std::string resource;
};

// Internal frame/control capability kept separate from the public rendering
// query/submit API so an alternative renderer can replace it explicitly.
class IRenderRuntimeControl
{
public:
    virtual ~IRenderRuntimeControl() = default;

    virtual void ResetQueuesBuilt() = 0;
    virtual void IncrementFrame() = 0;
    virtual void ResetRenderedCount() = 0;
    virtual void AddTime(float deltaTime) = 0;

    virtual void SetFilterLayerMask(uint32_t mask) = 0;
    virtual void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back) = 0;
    virtual void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less) = 0;
    virtual void SetInstanceBatching(bool enabled) = 0;
    virtual void SetFrustumCulling(bool enabled) = 0;
    virtual void SetSpatialCullingMode(SpatialCullingMode mode) = 0;
    virtual SpatialCullingMode GetActiveSpatialCullingMode() const = 0;
    virtual void SetRenderOrderEnabled(bool enabled) = 0;
    virtual void SetFrameDebugCapture(bool enabled) = 0;
    virtual void SetFrameDebugDrawLimit(uint64_t drawLimit) = 0;
    virtual std::vector<FrameDebugDrawCall> GetFrameDebugDrawCalls() const = 0;
};
