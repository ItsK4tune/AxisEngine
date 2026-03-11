#include <navigation/logic/navmesh_generator.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entt.hpp>
#include <resource/manager/resource_manager.h>
#include <render/interface/i_texture_manager.h>
#include <algorithm>
#include <map>
#include <unordered_map>

void NavMeshGenerator::Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources, const std::string& walkableTag)
{
    navMesh.needsRebuild = false;

    navMesh.vertices.clear();
    navMesh.triangles.clear();
    navMesh.nodes.clear();

    RawMeshData raw = GatherWalkableGeometry(scene, resources, walkableTag);
    if (raw.vertices.empty()) {
        LOGGER_WARN("NavMeshGenerator") << "No walkable geometry found! NavMesh will be empty.";
        return;
    }
    
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
        
        if (tri.normal.y > 0.3f) { // More lenient slope for terrain
            navMesh.triangles.push_back(tri);
        }
    }

    BuildConnectivity(navMesh);
}

NavMeshGenerator::RawMeshData NavMeshGenerator::GatherWalkableGeometry(Scene& scene, ResourceManager* resources, const std::string& walkableTag)
{
    RawMeshData result;

    LOGGER_INFO("NavMeshGenerator") << "Gathering walkable geometry for tag: " << walkableTag;

    // 1. Gather from Meshes with configurable walkable tag
    auto meshView = scene.registry.view<InfoComponent, MeshRendererComponent, WorldTransformComponent>();
    for (auto entity : meshView) {
        auto& info = meshView.get<InfoComponent>(entity);
        if (info.tag != walkableTag) continue;

        auto& renderer = meshView.get<MeshRendererComponent>(entity);
        auto& transform = meshView.get<WorldTransformComponent>(entity);

        if (!renderer.model) continue;

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

    // 2. Gather from Terrain (Check isWalkable flag)
    auto terrainView = scene.registry.view<TerrainComponent, PositionComponent>();
    for (auto entity : terrainView) {
        auto& terrain = terrainView.get<TerrainComponent>(entity);
        auto& pos = terrainView.get<PositionComponent>(entity);
        
        if (!terrain.isWalkable) continue;

        uint32_t baseIndex = (uint32_t)result.vertices.size();
        glm::vec3 size = terrain.terrainSize;

        int gridRes = 64; 
        float resStepX = size.x / (float)(gridRes - 1);
        float resStepZ = size.z / (float)(gridRes - 1);

        std::shared_ptr<Texture> tex = nullptr;
        if (resources && !terrain.heightMapName.empty()) {
            tex = resources->GetTexture(terrain.heightMapName);
        }

        if (tex && !tex->pixelData) {
            LOGGER_WARN("NavMeshGenerator") << "Heightmap found but pixelData is NULL for terrain " << (uint32_t)entity;
        }

        for (int z = 0; z < gridRes; ++z) {
            for (int x = 0; x < gridRes; ++x) {
                float hVal = 0.0f;
                if (tex && tex->pixelData) {
                    int texX = (int)((float)x / (float)(gridRes - 1) * (tex->width - 1));
                    int texZ = (int)((float)z / (float)(gridRes - 1) * (tex->height - 1));
                    int idx = (texZ * tex->width + texX) * tex->nrComponents;
                    hVal = (float)tex->pixelData[idx] / 255.0f * terrain.maxHeight;
                }
                // Generate vertex in world space
                result.vertices.push_back(pos.value + glm::vec3(x * resStepX, hVal, z * resStepZ));
            }
        }

        for (int z = 0; z < gridRes - 1; ++z) {
            for (int x = 0; x < gridRes - 1; ++x) {
                uint32_t i0 = baseIndex + (z * gridRes + x);
                uint32_t i1 = baseIndex + (z * gridRes + (x + 1));
                uint32_t i2 = baseIndex + ((z + 1) * gridRes + (x + 1));
                uint32_t i3 = baseIndex + ((z + 1) * gridRes + x);

                // Triangle 1: 0 -> 3 -> 2
                result.indices.push_back(i0);
                result.indices.push_back(i3);
                result.indices.push_back(i2);
                // Triangle 2: 0 -> 2 -> 1
                result.indices.push_back(i0);
                result.indices.push_back(i2);
                result.indices.push_back(i1);
            }
        }
        
        LOGGER_INFO("NavMeshGenerator") << "Generated " << gridRes << "x" << gridRes << " NavMesh grid for terrain " << (uint32_t)entity;
    }
    
    LOGGER_INFO("NavMeshGenerator") << "Total geometry gathered: vertices=" << result.vertices.size() << ", indices=" << result.indices.size();
    return result;
}

void NavMeshGenerator::BuildConnectivity(NavMeshComponent& navMesh)
{
    if (navMesh.triangles.empty()) return;

    // 1. Create nodes
    navMesh.nodes.clear();
    navMesh.nodes.reserve(navMesh.triangles.size());
    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i) {
        NavMeshNode node;
        node.position = navMesh.triangles[i].center;
        node.triangleIndex = i;
        navMesh.nodes.push_back(node);
    }

    // 2. Build edge map for O(N) connectivity
    // Map of edge (sorted vertex indices) -> list of triangle indices sharing it
    struct Edge {
        uint32_t v1, v2;
        bool operator==(const Edge& other) const {
            return v1 == other.v1 && v2 == other.v2;
        }
    };
    struct EdgeHash {
        std::size_t operator()(const Edge& e) const {
            return std::hash<uint32_t>{}(e.v1) ^ (std::hash<uint32_t>{}(e.v2) << 1);
        }
    };
    std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash> edgeMap;

    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i) {
        const auto& tri = navMesh.triangles[i];
        for (int k = 0; k < 3; ++k) {
            uint32_t va = tri.indices[k];
            uint32_t vb = tri.indices[(k + 1) % 3];
            Edge e = { std::min(va, vb), std::max(va, vb) };
            edgeMap[e].push_back(i);
        }
    }

    // 3. Connect nodes sharing edges
    for (auto const& [edge, triIndices] : edgeMap) {
        if (triIndices.size() >= 2) {
            for (size_t i = 0; i < triIndices.size(); ++i) {
                for (size_t j = i + 1; j < triIndices.size(); ++j) {
                    uint32_t idx_i = triIndices[i];
                    uint32_t idx_j = triIndices[j];
                    
                    // Add neighbors if not already added
                    if (std::find(navMesh.nodes[idx_i].neighbors.begin(), 
                                  navMesh.nodes[idx_i].neighbors.end(), idx_j) == navMesh.nodes[idx_i].neighbors.end()) {
                        navMesh.nodes[idx_i].neighbors.push_back(idx_j);
                    }
                    if (std::find(navMesh.nodes[idx_j].neighbors.begin(), 
                                  navMesh.nodes[idx_j].neighbors.end(), idx_i) == navMesh.nodes[idx_j].neighbors.end()) {
                        navMesh.nodes[idx_j].neighbors.push_back(idx_i);
                    }
                }
            }
        }
    }
}
