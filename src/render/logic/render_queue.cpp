#include <render/unit/render_queue.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <render/logic/frustum_culler.h>
#include <resource/unit/shader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <algorithm>

void RenderQueue::Sort()
{
    if (m_DeferredOpaqueQueue.size() > 1)
    {
        std::sort(m_DeferredOpaqueQueue.begin(), m_DeferredOpaqueQueue.end(),
                  [](const RenderItem& lhs, const RenderItem& rhs) { return lhs.sortKey < rhs.sortKey; });
    }

    if (m_ForwardOpaqueQueue.size() > 1)
    {
        std::sort(m_ForwardOpaqueQueue.begin(), m_ForwardOpaqueQueue.end(),
                  [](const RenderItem& lhs, const RenderItem& rhs) { return lhs.sortKey < rhs.sortKey; });
    }

    if (m_DepthOverlayQueue.size() > 1)
    {
        std::sort(m_DepthOverlayQueue.begin(), m_DepthOverlayQueue.end(),
                  [](const RenderItem& lhs, const RenderItem& rhs) { return lhs.sortKey < rhs.sortKey; });
    }

    if (m_TransparentQueue.size() > 1)
    {
        std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(),
                  [](const RenderItem& lhs, const RenderItem& rhs) { return lhs.sortKey < rhs.sortKey; });
    }
}
