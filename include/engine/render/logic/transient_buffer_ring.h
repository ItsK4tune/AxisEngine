#pragma once

#include <render/type/graphics_types.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

class IBufferManager;

// Triple-buffered transient uploads. On OpenGL 4.4+ this uses coherent,
// persistently-mapped storage guarded by per-segment fences. Other/custom
// backends transparently retain the orphan + subdata path.
class TransientBufferRing
{
public:
    struct UploadSlice
    {
        unsigned int buffer = 0;
        size_t offset = 0;
        size_t size = 0;
    };

    TransientBufferRing() = default;
    ~TransientBufferRing();
    TransientBufferRing(const TransientBufferRing&) = delete;
    TransientBufferRing& operator=(const TransientBufferRing&) = delete;

    static void SetGloballyEnabled(bool enabled);
    static bool IsGloballyEnabled();

    void Initialize(IBufferManager& manager, BufferType target, size_t segmentCapacity, size_t segmentCount = 3);
    void Shutdown();
    UploadSlice Upload(const void* data, size_t size);
    // Must be called after the draw/dispatch commands consuming the last slice.
    void Commit();

    unsigned int GetBuffer() const { return m_Buffer; }
    bool IsPersistent() const { return m_Persistent; }
    size_t GetSegmentCapacity() const { return m_SegmentCapacity; }

private:
    static std::atomic<bool> s_Enabled;
    static constexpr size_t MaxSegments = 4;

    IBufferManager* m_Manager = nullptr;
    BufferType m_Target = BufferType::ArrayBuffer;
    unsigned int m_Buffer = 0;
    void* m_Mapped = nullptr;
    size_t m_SegmentCapacity = 0;
    size_t m_SegmentStride = 0;
    size_t m_SegmentCount = 3;
    size_t m_CurrentSegment = 0;
    bool m_Persistent = false;
    bool m_UploadPending = false;
    std::array<uintptr_t, MaxSegments> m_Fences{};

    void Recreate(size_t requiredCapacity);
};
