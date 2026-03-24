#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>
#include <vector>

class Shader;
class Model;
#include <core/unit/aabb.h>
class FrustumCuller;
struct MaterialComponent;

struct RenderItem {
    entt::entity entity;
    Model* activeModel;
    Shader* activeShader;
    MaterialComponent* activeMaterial;
    glm::mat4 worldMatrix;
    AABB worldAABB;
    uint32_t layer;
    int renderOrder;
    float distSq;
    bool isTransparent;
    uint64_t sortKey;
};

class RenderQueue {
public:
    void Build(Scene& scene, float alpha, 
               const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
               bool occlusionCullEnabled, float distCullSq, 
               uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos);

    const std::vector<RenderItem>& GetOpaqueQueue() const { return m_OpaqueQueue; }
    const std::vector<RenderItem>& GetTransparentQueue() const { return m_TransparentQueue; }
    const std::vector<RenderItem>& GetShadowQueue() const { return m_ShadowQueue; }

private:
    std::vector<RenderItem> m_OpaqueQueue;
    std::vector<RenderItem> m_TransparentQueue;
    std::vector<RenderItem> m_ShadowQueue;
};