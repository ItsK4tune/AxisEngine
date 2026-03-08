#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>
#include <vector>

class FrustumCuller;
class Model;

struct RenderItem {
    entt::entity entity;
    Model* activeModel;
    glm::mat4 worldMatrix;
    AABB worldAABB;
    uint32_t layer;
    int renderOrder;
    float distSq;
    bool isTransparent;
};

class RenderQueue {
public:
    void Build(Scene& scene, float alpha, 
               const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
               bool occlusionCullEnabled, float distCullSq, 
               uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos);

    const std::vector<RenderItem>& GetOpaqueQueue() const { return m_RenderQueue; }
    const std::vector<RenderItem>& GetShadowQueue() const { return m_ShadowQueue; }

private:
    std::vector<RenderItem> m_RenderQueue;
    std::vector<RenderItem> m_ShadowQueue;
};