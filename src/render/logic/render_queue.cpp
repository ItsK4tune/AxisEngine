#include <ecs/unit/media_components.h>
#include <ecs/unit/core_components.h>
#include <render/unit/render_queue.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <render/logic/frustum_culler.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <algorithm>

void RenderQueue::Sort() {
    std::sort(m_OpaqueQueue.begin(), m_OpaqueQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });

    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });
}
