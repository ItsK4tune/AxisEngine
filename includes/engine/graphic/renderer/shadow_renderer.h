#pragma once

#include <scene/scene.h>
#include <graphic/renderer/shadow.h>
#include <graphic/renderer/frustum.h>

class ResourceManager;
class Shader;

class ShadowRenderer
{
public:
    void Init(ResourceManager &res);
    void Shutdown();
    void RenderShadows(Scene &scene);

    void SetEnableShadows(bool enable) { m_EnableShadows = enable; }
    void SetShadowMode(int mode) { m_ShadowMode = mode; }
    bool IsShadowsEnabled() const { return m_EnableShadows; }
    int GetShadowMode() const { return m_ShadowMode; }
    
    void SetShadowProjectionSize(float size) { m_ShadowProjectionSize = size; }
    void SetShadowFrustumCulling(bool enable) { m_ShadowFrustumCullingEnabled = enable; }
    void SetShadowDistanceCulling(float distance) { m_ShadowDistanceCullingSq = distance * distance; }

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
    int m_ShadowMode = 1; // 0=None, 1=Once, 2=All
    
    float m_ShadowProjectionSize = 20.0f;
    bool m_ShadowFrustumCullingEnabled = true;
    float m_ShadowDistanceCullingSq = 10000.0f; // Default 100^2
};
