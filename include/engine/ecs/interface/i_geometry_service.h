#pragma once
#include <core/interface/i_base_system.h>
#include <cstdint>
#include <vector>

class GBuffer;

class IGeometryService : virtual public IBaseSystem
{
public:
    virtual ~IGeometryService() = default;

    virtual GBuffer& GetGBuffer() = 0;
    virtual void BindGBufferForWriting() = 0;
    virtual void UnbindGBuffer() = 0;

    virtual bool IsDeferredRenderingEnabled() const = 0;
    virtual void BeginDecalPass() = 0;
    virtual void EndDecalPass(uint32_t fbo) = 0;

    virtual uint32_t GetGBufferWidth() const = 0;
    virtual uint32_t GetGBufferHeight() const = 0;
    virtual uint32_t GetPickingBufferWidth() const = 0;
    virtual uint32_t GetPickingBufferHeight() const = 0;

    virtual unsigned int GetGBufferDepth() const = 0;
    virtual unsigned int GetGBufferID() const = 0;
    // Request the optional entity-ID attachment for the next geometry pass.
    // Call each frame while an editor/picking consumer needs it.
    virtual void RequestEntityIdBuffer() = 0;
    virtual bool ReadEntityId(int x, int y, uint32_t& entityId) const = 0;
    virtual bool ReadEntityIds(int x, int y, int width, int height, std::vector<uint32_t>& entityIds) const = 0;
    virtual unsigned int GetGBufferNormal() const = 0;
};
