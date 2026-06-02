#include <render/unit/render_queue.h>
#include <algorithm>

namespace
{
bool SortKeyLess(const RenderItem& lhs, const RenderItem& rhs)
{
    return lhs.sortKey < rhs.sortKey;
}
}  // namespace

void RenderQueue::Sort()
{
    if (m_DeferredOpaqueQueue.size() > 1)
    {
        std::sort(m_DeferredOpaqueQueue.begin(), m_DeferredOpaqueQueue.end(), SortKeyLess);
    }

    if (m_ForwardOpaqueQueue.size() > 1)
    {
        std::sort(m_ForwardOpaqueQueue.begin(), m_ForwardOpaqueQueue.end(), SortKeyLess);
    }

    if (m_DepthOverlayQueue.size() > 1)
    {
        std::sort(m_DepthOverlayQueue.begin(), m_DepthOverlayQueue.end(), SortKeyLess);
    }

    if (m_TransparentQueue.size() > 1)
    {
        std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), SortKeyLess);
    }
}
