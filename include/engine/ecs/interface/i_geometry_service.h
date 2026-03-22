#pragma once
#include <ecs/interface/i_base_system.h>

#include <cstdint>
class GBuffer;

/**
 * @brief Service interface for geometry-related operations (G-Buffer).
 */
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
    
    virtual unsigned int GetGBufferDepth() const = 0;
    virtual unsigned int GetGBufferID() const = 0;
    virtual unsigned int GetGBufferPosition() const = 0;
};
