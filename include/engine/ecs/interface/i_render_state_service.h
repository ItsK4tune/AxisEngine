#pragma once

#include <glm/glm.hpp>
#include <render/type/graphics_types.h>

enum class AntiAliasingMode;

class IRenderStateService
{
public:
    virtual ~IRenderStateService() = default;

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


    virtual unsigned int GetWhiteTexture() const = 0;
    virtual unsigned int GetBlackTexture() const = 0;
    virtual unsigned int GetFlatNormalTexture() const = 0;

    virtual void UpdateGlobalLightData(const GPUGlobalLightData& data) = 0;
};
