#include <ecs/unit/core_components.h>
#include <render/unit/render_queue.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <render/logic/frustum_culler.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <algorithm>

void RenderQueue::Build(Scene& scene, float alpha, 
                        const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
                        bool occlusionCullEnabled, float distCullSq, 
                        uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos) {
    
    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();
    m_ShadowQueue.clear();

    auto renderView = scene.registry.view<WorldTransformComponent, MeshRendererComponent>();
    
    static int frameCounter = 0;
    bool debugFrame = (frameCounter++ % 60 == 0);
    int totalMeshes = 0;
    int skippedByModel = 0;
    int skippedByLayer = 0;
    int skippedByFrustum = 0;

    for (auto entity : renderView) {
        totalMeshes++;
        auto& world = renderView.get<WorldTransformComponent>(entity);
        auto& renderer = renderView.get<MeshRendererComponent>(entity);
        if (!renderer.model) {
            skippedByModel++;
            continue;
        }

        glm::mat4 modelMatrix = world.GetInterpolated(alpha);
        float distSqResult = glm::length2(camPos - glm::vec3(modelMatrix[3]));

        if (distCullSq > 0.0f && distSqResult > distCullSq) continue;

        AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);
        if (frustumCullEnabled) {
            if (!frustumCuller.IsVisible(worldAABB.minBound, worldAABB.maxBound)) {
                skippedByFrustum++;
                continue;
            }
        }

        uint32_t layer = 1;
        if (auto* info = scene.registry.try_get<InfoComponent>(entity)) layer = info->layer;
        
        if ((filterMask & layer) == 0 || (camMask & layer) == 0) {
            skippedByLayer++;
            continue;
        }

        Model* activeModel = renderer.model.get();
        Shader* itemShader = renderer.shader.lock().get();

        if (auto* lod = scene.registry.try_get<LODComponent>(entity)) {
            for (int j = 0; j < (int)lod->lodDistancesSq.size(); ++j) {
                if (distSqResult > lod->lodDistancesSq[j] && j < (int)lod->lodModels.size() && lod->lodModels[j]) {
                    activeModel = lod->lodModels[j].get();
                } else break;
            }
        }

        if (occlusionCullEnabled) {
            if (auto* occ = scene.registry.try_get<OcclusionComponent>(entity)) {
                if (!occ->isVisible) continue;
            }
        }

        bool isTransparent = false;
        auto* material = scene.registry.try_get<MaterialComponent>(entity);
        if (material) {
            if (material->desc.opacity < 1.0f) isTransparent = true;
        }

        uint64_t key = 0;
        uint64_t sId = itemShader ? itemShader->getID() : 0;
        if (!isTransparent) {

            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            uint64_t o = (uint64_t)(renderer.order & 0xFF) << 48;
            uint64_t s = (uint64_t)(sId & 0xFFFF) << 32;
            uint64_t m = (uint64_t)((uintptr_t)material & 0xFFFF) << 16;
            uint64_t mod = (uint64_t)((uintptr_t)activeModel & 0xFF) << 8;
            uint64_t d = (uint64_t)(glm::clamp(distSqResult * 0.1f, 0.0f, 255.0f)) & 0xFF;
            key = l | o | s | m | mod | d;
        } else {

            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            float invDepth = 1000000.0f - distSqResult; 
            if (invDepth < 0) invDepth = 0;
            uint64_t d = (uint64_t)(invDepth) & 0x00FFFFFFFFFFFFFFULL;
            key = l | d;
        }

        RenderItem item = {entity, activeModel, itemShader, material, modelMatrix, worldAABB, layer, renderer.order, distSqResult, isTransparent, key};
        if (isTransparent) m_TransparentQueue.push_back(item);
        else m_OpaqueQueue.push_back(item);
        
        if (renderer.castShadow) m_ShadowQueue.push_back(item);
    }


    std::sort(m_OpaqueQueue.begin(), m_OpaqueQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });


    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });
}
