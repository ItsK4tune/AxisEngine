#pragma once

#include <render/logic/frustum.h>
#include <render/logic/shadow.h>
#include <resource/interface/i_resource_libraries.h>
#include <scene/logic/scene.h>
#include <vector>

class ResourceManager;
struct RenderItem;

class ShadowRenderer
{
public:
    void Initialize(IGraphicsContext& context, IShaderLibrary &shaderLib);
    void Shutdown();
    void RenderShadows(Scene &scene, const std::vector<RenderItem>& shadowQueue);

    void SetEnableShadows(bool enable) { m_EnableShadows = enable; }
    void SetShadowMode(int mode) { m_ShadowMode = mode; }
    bool IsShadowsEnabled() const { return m_EnableShadows; }
    int GetShadowMode() const { return m_ShadowMode; }

    void SetShadowProjectionSize(float size) { m_ShadowProjectionSize = size; }
    void SetShadowFrustumCulling(bool enable) { m_ShadowFrustumCullingEnabled = enable; }
    void SetShadowDistanceCulling(float distance) { m_ShadowDistanceCullingSq = distance * distance; }
    void SetShadowBias(float bias) { m_ShadowBias = bias; }
    void SetShadowSoftness(int softness) { m_ShadowSoftness = softness; }

    float GetShadowBias() const { return m_ShadowBias; }
    int GetShadowSoftness() const { return m_ShadowSoftness; }

    Shadow &GetShadow() { return m_Shadow; }
    const glm::mat4* GetLightSpaceMatrices() const { return m_LightSpaceMatrixDir; }
    const glm::mat4* GetLightSpaceMatricesSpot() const { return m_LightSpaceMatrixSpot; }
    float GetFarPlanePoint() const { return m_FarPlanePoint; }
    float GetFarPlaneSpot() const { return m_FarPlaneSpot; }

private:
    Shadow m_Shadow;

    glm::mat4 m_LightSpaceMatrixDir[Shadow::MAX_DIR_LIGHTS_SHADOW];
    glm::mat4 m_LightSpaceMatrixSpot[Shadow::MAX_SPOT_LIGHTS_SHADOW];
    float m_FarPlanePoint = 100.0f;
    float m_FarPlaneSpot = 100.0f;
    bool m_EnableShadows = true;
    int m_ShadowMode = 1;

    float m_ShadowProjectionSize = 20.0f;
    bool m_ShadowFrustumCullingEnabled = true;
    float m_ShadowDistanceCullingSq = 10000.0f;
    float m_ShadowBias = 0.005f;
    int m_ShadowSoftness = 1;
    bool m_DirLightLimitWarned = false;
};