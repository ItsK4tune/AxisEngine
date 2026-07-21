#include <render/unit/render_queue.h>
#include <algorithm>
#include <bit>
#include <functional>

namespace
{
bool DepthOverlayLess(const RenderItem& lhs, const RenderItem& rhs)
{
    if (lhs.layer != rhs.layer)
        return lhs.layer < rhs.layer;
    if (lhs.renderOrder != rhs.renderOrder)
        return lhs.renderOrder > rhs.renderOrder;
    return lhs.entityId < rhs.entityId;
}

bool TransparentBackToFront(const RenderItem& lhs, const RenderItem& rhs)
{
    if (lhs.layer != rhs.layer)
        return lhs.layer < rhs.layer;
    if (lhs.renderOrder != rhs.renderOrder)
        return lhs.renderOrder < rhs.renderOrder;
    if (lhs.distanceSq != rhs.distanceSq)
        return lhs.distanceSq > rhs.distanceSq;
    return lhs.entityId < rhs.entityId;
}

bool OpaqueStateLess(const RenderItem& lhs, const RenderItem& rhs)
{
    if (lhs.layer != rhs.layer)
        return lhs.layer < rhs.layer;
    if (lhs.renderOrder != rhs.renderOrder)
        return lhs.renderOrder < rhs.renderOrder;
    if (lhs.shader != rhs.shader)
        return std::less<const Shader*>{}(lhs.shader, rhs.shader);
    if (lhs.model != rhs.model)
        return std::less<const Model*>{}(lhs.model, rhs.model);
    if (lhs.materialBatchKey != rhs.materialBatchKey)
        return lhs.materialBatchKey < rhs.materialBatchKey;
    if (lhs.renderMode != rhs.renderMode)
        return static_cast<int>(lhs.renderMode) < static_cast<int>(rhs.renderMode);
    if (lhs.receiveShadow != rhs.receiveShadow)
        return lhs.receiveShadow < rhs.receiveShadow;
    if (lhs.castShadow != rhs.castShadow)
        return lhs.castShadow < rhs.castShadow;
    if (lhs.hasAnimation != rhs.hasAnimation)
        return lhs.hasAnimation < rhs.hasAnimation;
    if (lhs.reflection != rhs.reflection)
        return std::less<const ReflectiveComponent*>{}(lhs.reflection, rhs.reflection);
    if (lhs.probeIndex != rhs.probeIndex)
        return lhs.probeIndex < rhs.probeIndex;
    if (lhs.ignoreDepth != rhs.ignoreDepth)
        return lhs.ignoreDepth < rhs.ignoreDepth;
    for (int component = 0; component < 4; ++component)
    {
        const uint32_t leftBits = std::bit_cast<uint32_t>(lhs.tintColor[component]);
        const uint32_t rightBits = std::bit_cast<uint32_t>(rhs.tintColor[component]);
        if (leftBits != rightBits)
            return leftBits < rightBits;
    }
    if (lhs.distanceSq != rhs.distanceSq)
        return lhs.distanceSq < rhs.distanceSq;
    return lhs.entityId < rhs.entityId;
}

bool OpaqueBatchStateLess(const RenderItem& lhs, const RenderItem& rhs)
{
    if (lhs.layer != rhs.layer)
        return lhs.layer < rhs.layer;
    if (lhs.renderOrder != rhs.renderOrder)
        return lhs.renderOrder < rhs.renderOrder;
    if (lhs.shader != rhs.shader)
        return std::less<const Shader*>{}(lhs.shader, rhs.shader);
    if (lhs.model != rhs.model)
        return std::less<const Model*>{}(lhs.model, rhs.model);
    if (lhs.materialBatchKey != rhs.materialBatchKey)
        return lhs.materialBatchKey < rhs.materialBatchKey;
    if (lhs.renderMode != rhs.renderMode)
        return static_cast<int>(lhs.renderMode) < static_cast<int>(rhs.renderMode);
    if (lhs.receiveShadow != rhs.receiveShadow)
        return lhs.receiveShadow < rhs.receiveShadow;
    if (lhs.castShadow != rhs.castShadow)
        return lhs.castShadow < rhs.castShadow;
    if (lhs.hasAnimation != rhs.hasAnimation)
        return lhs.hasAnimation < rhs.hasAnimation;
    if (lhs.reflection != rhs.reflection)
        return std::less<const ReflectiveComponent*>{}(lhs.reflection, rhs.reflection);
    if (lhs.probeIndex != rhs.probeIndex)
        return lhs.probeIndex < rhs.probeIndex;
    if (lhs.ignoreDepth != rhs.ignoreDepth)
        return lhs.ignoreDepth < rhs.ignoreDepth;
    for (int component = 0; component < 4; ++component)
    {
        const uint32_t leftBits = std::bit_cast<uint32_t>(lhs.tintColor[component]);
        const uint32_t rightBits = std::bit_cast<uint32_t>(rhs.tintColor[component]);
        if (leftBits != rightBits)
            return leftBits < rightBits;
    }
    return false;
}
}  // namespace

void RenderQueue::Sort(bool instanceBatchingEnabled)
{
    const auto opaqueLess = instanceBatchingEnabled ? OpaqueBatchStateLess : OpaqueStateLess;
    if (m_DeferredOpaqueQueue.size() > 1)
    {
        if (!std::is_sorted(m_DeferredOpaqueQueue.begin(), m_DeferredOpaqueQueue.end(), opaqueLess))
            std::sort(m_DeferredOpaqueQueue.begin(), m_DeferredOpaqueQueue.end(), opaqueLess);
    }

    if (m_ForwardOpaqueQueue.size() > 1)
    {
        if (!std::is_sorted(m_ForwardOpaqueQueue.begin(), m_ForwardOpaqueQueue.end(), opaqueLess))
            std::sort(m_ForwardOpaqueQueue.begin(), m_ForwardOpaqueQueue.end(), opaqueLess);
    }

    if (m_DepthOverlayQueue.size() > 1)
    {
        std::sort(m_DepthOverlayQueue.begin(), m_DepthOverlayQueue.end(), DepthOverlayLess);
    }

    if (m_TransparentQueue.size() > 1)
    {
        std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), TransparentBackToFront);
    }
}
