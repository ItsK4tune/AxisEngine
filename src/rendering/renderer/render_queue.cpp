#include <rendering/renderer/render_queue.h>
#include <rendering/renderer/frustum_culler.h>
#include <ecs/component.h>
#include <ecs/components/occlusion_component.h>
#include <rendering/core/shader.h>
#include <algorithm>

void RenderQueue::Build(Scene& scene, float alpha, 
                        const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
                        bool occlusionCullEnabled, float distCullSq, 
                        uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos) {
    
    m_RenderQueue.clear();
    m_ShadowQueue.clear();

    std::vector<entt::entity> visibleEntities;
    frustumCuller.Cull(scene, frustumCullEnabled, visibleEntities, alpha);

    for (auto entity : visibleEntities) {
        if (!scene.registry.all_of<TransformComponent, MeshRendererComponent>(entity)) continue;

        auto& transform = scene.registry.get<TransformComponent>(entity);
        auto& renderer = scene.registry.get<MeshRendererComponent>(entity);

        if (!renderer.model || renderer.shader.expired()) continue;

        glm::mat4 modelMatrix = transform.GetInterpolatedWorldMatrix(scene.registry, alpha);

        float distSq = 0.0f;
        glm::vec3 worldMin = modelMatrix * glm::vec4(renderer.model->aabb.minBound, 1.0f);
        glm::vec3 worldMax = modelMatrix * glm::vec4(renderer.model->aabb.maxBound, 1.0f);

        float dx = std::max(worldMin.x - camPos.x, std::max(0.0f, camPos.x - worldMax.x));
        float dy = std::max(worldMin.y - camPos.y, std::max(0.0f, camPos.y - worldMax.y));
        float dz = std::max(worldMin.z - camPos.z, std::max(0.0f, camPos.z - worldMax.z));
        distSq = dx * dx + dy * dy + dz * dz;

        if (distCullSq > 0.0f && distSq > distCullSq) continue;

        Model* activeModel = renderer.model.get();
        if (auto* lod = scene.registry.try_get<LODComponent>(entity)) {
            for (int i = 0; i < (int)lod->lodDistancesSq.size(); ++i) {
                if (distSq > lod->lodDistancesSq[i] && i < (int)lod->lodModels.size() && lod->lodModels[i]) {
                    activeModel = lod->lodModels[i].get();
                } else break;
            }
        }

        uint32_t layer = 1;
        if (auto* info = scene.registry.try_get<InfoComponent>(entity)) layer = info->layer;

        if ((filterMask & layer) == 0 || (camMask & layer) == 0) continue;

        if (occlusionCullEnabled) {
            if (auto* occ = scene.registry.try_get<OcclusionComponent>(entity)) {
                if (!occ->isVisible) continue;
            }
        }

        bool isTransparent = false;
        if (auto* mat = scene.registry.try_get<MaterialComponent>(entity)) {
            if (mat->desc.opacity < 1.0f) isTransparent = true;
        }

        m_RenderQueue.push_back({entity, activeModel, modelMatrix, layer, renderer.order, distSq, isTransparent});
        if (renderer.castShadow) m_ShadowQueue.push_back({entity, activeModel, modelMatrix, layer, renderer.order, distSq, isTransparent});
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [&scene](const RenderItem& lhs, const RenderItem& rhs) {
        if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
        if (lhs.isTransparent != rhs.isTransparent) return !lhs.isTransparent;

        if (!lhs.isTransparent) {
            if (lhs.renderOrder != rhs.renderOrder) return lhs.renderOrder < rhs.renderOrder;

            auto& lRenderer = scene.registry.get<MeshRendererComponent>(lhs.entity);
            auto& rRenderer = scene.registry.get<MeshRendererComponent>(rhs.entity);

            auto lShader = lRenderer.shader.lock();
            auto rShader = rRenderer.shader.lock();
            unsigned int lID = lShader ? lShader->getID() : 0;
            unsigned int rID = rShader ? rShader->getID() : 0;
            if (lID != rID) return lID < rID;
            
            auto lMat = scene.registry.try_get<MaterialComponent>(lhs.entity);
            auto rMat = scene.registry.try_get<MaterialComponent>(rhs.entity);
            if (lMat != rMat) return lMat < rMat;

            return lhs.activeModel < rhs.activeModel;
        } else {
            return lhs.distSq > rhs.distSq;
        }
    });
}
