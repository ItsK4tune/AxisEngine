#include <render/logic/transient_buffer_ring.h>
#include <core/logic/runtime_profiler.h>
#include <render/interface/i_buffer_manager.h>
#include <algorithm>
#include <cstring>
#include <limits>

std::atomic<bool> TransientBufferRing::s_Enabled{true};

namespace
{
size_t AlignUp(size_t value, size_t alignment)
{
    alignment = (std::max)(size_t{1}, alignment);
    const size_t remainder = value % alignment;
    return remainder == 0 ? value : value + alignment - remainder;
}

size_t GrowCapacity(size_t current, size_t required)
{
    size_t result = (std::max)(size_t{256}, current);
    while (result < required && result <= (std::numeric_limits<size_t>::max)() / 2)
        result *= 2;
    return (std::max)(result, required);
}
}  // namespace

TransientBufferRing::~TransientBufferRing()
{
    Shutdown();
}

void TransientBufferRing::SetGloballyEnabled(bool enabled)
{
    s_Enabled.store(enabled, std::memory_order_release);
}

bool TransientBufferRing::IsGloballyEnabled()
{
    return s_Enabled.load(std::memory_order_acquire);
}

void TransientBufferRing::Initialize(IBufferManager& manager, BufferType target, size_t segmentCapacity,
                                     size_t segmentCount)
{
    Shutdown();
    m_Manager = &manager;
    m_Target = target;
    m_SegmentCapacity = (std::max)(size_t{1}, segmentCapacity);
    m_SegmentCount = std::clamp(segmentCount, size_t{2}, MaxSegments);
    Recreate(m_SegmentCapacity);
}

void TransientBufferRing::Shutdown()
{
    if (m_Manager)
    {
        for (size_t index = 0; index < m_SegmentCount; ++index)
        {
            if (m_Fences[index])
                m_Manager->WaitAndDeleteGpuFence(m_Fences[index]);
        }
        if (m_Buffer)
        {
            m_Manager->BindBuffer(m_Target, m_Buffer);
            if (m_Mapped)
                m_Manager->UnmapPersistentStorage(m_Target);
            m_Manager->DeleteBuffer(m_Buffer);
        }
    }
    m_Fences.fill(0);
    m_Buffer = 0;
    m_Mapped = nullptr;
    m_CurrentSegment = 0;
    m_Persistent = false;
    m_UploadPending = false;
    m_Manager = nullptr;
}

void TransientBufferRing::Recreate(size_t requiredCapacity)
{
    if (!m_Manager)
        return;
    for (size_t index = 0; index < m_SegmentCount; ++index)
    {
        if (m_Fences[index])
        {
            m_Manager->WaitAndDeleteGpuFence(m_Fences[index]);
            m_Fences[index] = 0;
        }
    }
    if (m_Buffer)
    {
        m_Manager->BindBuffer(m_Target, m_Buffer);
        if (m_Mapped)
            m_Manager->UnmapPersistentStorage(m_Target);
        m_Manager->DeleteBuffer(m_Buffer);
    }

    m_SegmentCapacity = GrowCapacity(m_SegmentCapacity, requiredCapacity);
    m_SegmentStride = AlignUp(m_SegmentCapacity, m_Manager->GetBufferOffsetAlignment(m_Target));
    m_Buffer = m_Manager->CreateBuffer();
    m_Manager->BindBuffer(m_Target, m_Buffer);
    m_Persistent = IsGloballyEnabled() && m_Manager->SupportsPersistentMapping();
    m_Mapped = m_Persistent
                   ? m_Manager->AllocatePersistentStorage(m_Target, m_SegmentStride * m_SegmentCount)
                   : nullptr;
    if (m_Persistent && !m_Mapped)
    {
        // BufferStorage is immutable; recreate a fresh handle for fallback.
        m_Manager->DeleteBuffer(m_Buffer);
        m_Buffer = m_Manager->CreateBuffer();
        m_Manager->BindBuffer(m_Target, m_Buffer);
        m_Persistent = false;
    }
    if (!m_Persistent)
        m_Manager->BufferData(m_Target, m_SegmentCapacity, nullptr, BufferUsage::StreamDraw);
    m_CurrentSegment = 0;
    m_UploadPending = false;
}

TransientBufferRing::UploadSlice TransientBufferRing::Upload(const void* data, size_t size)
{
    if (!m_Manager || !data || size == 0)
        return {};
    const bool wantsPersistent = IsGloballyEnabled() && m_Manager->SupportsPersistentMapping();
    if (size > m_SegmentCapacity || wantsPersistent != m_Persistent)
        Recreate(size);

    m_Manager->BindBuffer(m_Target, m_Buffer);
    if (m_Persistent)
    {
        if (m_Fences[m_CurrentSegment])
        {
            m_Manager->WaitAndDeleteGpuFence(m_Fences[m_CurrentSegment]);
            m_Fences[m_CurrentSegment] = 0;
        }
        const size_t offset = m_CurrentSegment * m_SegmentStride;
        std::memcpy(static_cast<std::byte*>(m_Mapped) + offset, data, size);
        m_Manager->FlushPersistentWrites();
        RuntimeProfiler::Instance().AddUploadBytes(size);
        m_UploadPending = true;
        return {m_Buffer, offset, size};
    }

    m_Manager->BufferData(m_Target, m_SegmentCapacity, nullptr, BufferUsage::StreamDraw);
    m_Manager->BufferSubData(m_Target, 0, size, data);
    m_UploadPending = true;
    return {m_Buffer, 0, size};
}

void TransientBufferRing::Commit()
{
    if (!m_Manager || !m_UploadPending)
        return;
    if (m_Persistent)
    {
        m_Fences[m_CurrentSegment] = m_Manager->InsertGpuFence();
        m_CurrentSegment = (m_CurrentSegment + 1) % m_SegmentCount;
    }
    m_UploadPending = false;
}
