#include <navigation/logic/navmesh_generator.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <map>

void NavMeshGenerator::Generate(Scene& scene, NavMeshComponent& navMesh)
{
    navMesh.vertices.clear();
    navMesh.triangles.clear();
    navMesh.nodes.clear();

    RawMeshData raw = GatherWalkableGeometry(scene);
    if (raw.vertices.empty()) return;

    navMesh.vertices = raw.vertices;
    
    for (size_t i = 0; i < raw.indices.size(); i += 3) {
        NavMeshTriangle tri;
        tri.indices[0] = raw.indices[i];
        tri.indices[1] = raw.indices[i+1];
        tri.indices[2] = raw.indices[i+2];
        
        const glm::vec3& v0 = navMesh.vertices[tri.indices[0]];
        const glm::vec3& v1 = navMesh.vertices[tri.indices[1]];
        const glm::vec3& v2 = navMesh.vertices[tri.indices[2]];
        
        tri.center = (v0 + v1 + v2) / 3.0f;
        tri.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        
        // Filter by slope (e.g., only flat-ish surfaces)
        if (tri.normal.y > 0.7f) {
            navMesh.triangles.push_back(tri);
        }
    }

    BuildConnectivity(navMesh);
    navMesh.needsRebuild = false;
}

NavMeshGenerator::RawMeshData NavMeshGenerator::GatherWalkableGeometry(Scene& scene)
{
    RawMeshData result;
    auto view = scene.registry.view<InfoComponent, MeshRendererComponent, WorldTransformComponent>();

    LOGGER_INFO("NavMeshGenerator") << "Gathering walkable geometry...";

    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (info.tag != "Walkable") continue;

        auto& renderer = view.get<MeshRendererComponent>(entity);
        auto& transform = view.get<WorldTransformComponent>(entity);

        if (!renderer.model) {
            LOGGER_WARN("NavMeshGenerator") << "Entity " << info.name << " has 'Walkable' tag but NULL model!";
            continue;
        }

        LOGGER_INFO("NavMeshGenerator") << "Processing Walkable entity: " << info.name << " with " << renderer.model->meshes.size() << " meshes";

        for (const auto& mesh : renderer.model->meshes) {
            uint32_t baseIndex = (uint32_t)result.vertices.size();
            
            for (const auto& v : mesh.vertices) {
                glm::vec4 worldPos = transform.worldMatrix * glm::vec4(v.Position, 1.0f);
                result.vertices.push_back(glm::vec3(worldPos));
            }

            for (unsigned int idx : mesh.indices) {
                result.indices.push_back(baseIndex + idx);
            }
        }
    }
    
    LOGGER_INFO("NavMeshGenerator") << "Total geometry gathered: vertices=" << result.vertices.size() << ", indices=" << result.indices.size();
    return result;
}

void NavMeshGenerator::BuildConnectivity(NavMeshComponent& navMesh)
{
    // Simplified node per triangle center
    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i) {
        NavMeshNode node;
        node.position = navMesh.triangles[i].center;
        node.triangleIndex = i;
        navMesh.nodes.push_back(node);
    }

    // Connect adjacent triangles (sharing 2 vertices)
    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i) {
        for (uint32_t j = i + 1; j < (uint32_t)navMesh.triangles.size(); ++j) {
            int shared = 0;
            for (int k = 0; k < 3; ++k) {
                for (int l = 0; l < 3; ++l) {
                    if (navMesh.triangles[i].indices[k] == navMesh.triangles[j].indices[l]) {
                        shared++;
                    }
                }
            }

            if (shared >= 2) {
                navMesh.nodes[i].neighbors.push_back(j);
                navMesh.nodes[j].neighbors.push_back(i);
            }
        }
    }
}
