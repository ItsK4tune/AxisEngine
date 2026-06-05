#include <navigation/logic/navmesh_generator.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/resource_manager.h>
#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

void NavMeshGenerator::Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources,
                                const std::vector<std::string>& walkableTags, const std::vector<std::string>& carveTags)
{
    navMesh.needsRebuild = false;

    navMesh.vertices.clear();
    navMesh.triangles.clear();
    navMesh.nodes.clear();

    RawMeshData raw = GatherWalkableGeometry(scene, resources, walkableTags, carveTags,
                                             navMesh.terrainGridResolution);
    if (raw.vertices.empty())
    {
        LOGGER_WARN("NavMeshGenerator") << "No walkable geometry found! NavMesh will be empty.";
        return;
    }

    struct ObstacleAABB
    {
        glm::vec3 min, max;
    };
    std::vector<ObstacleAABB> obstacles;
    auto infoView = scene.registry.view<InfoComponent, WorldTransformComponent>();
    for (auto ent : infoView)
    {
        auto& info = infoView.get<InfoComponent>(ent);
        bool isObstacle = false;
        for (const auto& tag : carveTags)
        {
            if (info.tag == tag)
            {
                isObstacle = true;
                break;
            }
        }
        if (!isObstacle)
            continue;

        auto& transform = infoView.get<WorldTransformComponent>(ent);
        if (scene.registry.all_of<MeshRendererComponent>(ent))
        {
            auto& renderer = scene.registry.get<MeshRendererComponent>(ent);
            glm::vec3 localMin(-0.5f);
            glm::vec3 localMax(0.5f);
            if (renderer.model)
            {
                localMin = renderer.model->aabb.minBound;
                localMax = renderer.model->aabb.maxBound;
            }
            glm::vec3 worldMin = glm::vec3(transform.worldMatrix * glm::vec4(localMin, 1.0f));
            glm::vec3 worldMax = glm::vec3(transform.worldMatrix * glm::vec4(localMax, 1.0f));

            glm::vec3 actualMin = glm::min(worldMin, worldMax);
            glm::vec3 actualMax = glm::max(worldMin, worldMax);

            actualMin.x -= navMesh.carveAgentRadius;
            actualMin.z -= navMesh.carveAgentRadius;
            actualMax.x += navMesh.carveAgentRadius;
            actualMax.z += navMesh.carveAgentRadius;
            obstacles.push_back({actualMin, actualMax});
        }
    }

    navMesh.vertices = raw.vertices;

    for (size_t i = 0; i < raw.indices.size(); i += 3)
    {
        size_t triIdx = i / 3;
        NavMeshTriangle tri;
        tri.indices[0] = raw.indices[i];
        tri.indices[1] = raw.indices[i + 1];
        tri.indices[2] = raw.indices[i + 2];
        tri.tag = (triIdx < raw.tags.size()) ? raw.tags[triIdx] : "walkable";

        const glm::vec3& v0 = navMesh.vertices[tri.indices[0]];
        const glm::vec3& v1 = navMesh.vertices[tri.indices[1]];
        const glm::vec3& v2 = navMesh.vertices[tri.indices[2]];

        tri.center = (v0 + v1 + v2) / 3.0f;
        tri.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        if (tri.normal.y > navMesh.walkableNormalY)
        {
            bool obstructed = false;
            for (const auto& obs : obstacles)
            {
                if (tri.center.x >= obs.min.x && tri.center.x <= obs.max.x && tri.center.z >= obs.min.z &&
                    tri.center.z <= obs.max.z && tri.center.y >= obs.min.y - navMesh.carveHeightPadding &&
                    tri.center.y <= obs.max.y + navMesh.carveHeightPadding)
                {
                    obstructed = true;
                    break;
                }
            }
            if (!obstructed)
            {
                navMesh.triangles.push_back(tri);
            }
        }
    }

    BuildConnectivity(navMesh);
}

NavMeshGenerator::RawMeshData NavMeshGenerator::GatherWalkableGeometry(Scene& scene, ResourceManager* resources,
                                                                       const std::vector<std::string>& walkableTags,
                                                                       const std::vector<std::string>& carveTags,
                                                                       int terrainGridResolution)
{
    RawMeshData result;

    LOGGER_INFO("NavMeshGenerator") << "Gathering walkable geometry for " << walkableTags.size() << " tags.";

    auto meshView = scene.registry.view<InfoComponent, MeshRendererComponent, WorldTransformComponent>();
    for (auto entity : meshView)
    {
        auto& info = meshView.get<InfoComponent>(entity);

        bool isWalkable = false;
        for (const auto& tag : walkableTags)
        {
            if (info.tag == tag)
            {
                isWalkable = true;
                break;
            }
        }
        if (!isWalkable)
            continue;

        auto& renderer = meshView.get<MeshRendererComponent>(entity);
        auto& transform = meshView.get<WorldTransformComponent>(entity);

        if (!renderer.model)
            continue;

        for (const auto& mesh : renderer.model->meshes)
        {
            uint32_t baseIndex = (uint32_t)result.vertices.size();
            for (size_t vIdx = 0; vIdx < mesh.m_VertexCount; ++vIdx)
            {
                const float* posPtr =
                    reinterpret_cast<const float*>(mesh.m_VertexData.data() + vIdx * mesh.m_VertexStride);
                glm::vec3 pos(posPtr[0], posPtr[1], posPtr[2]);
                glm::vec4 worldPos = transform.worldMatrix * glm::vec4(pos, 1.0f);
                result.vertices.push_back(glm::vec3(worldPos));
            }
            for (unsigned int idx : mesh.indices)
            {
                result.indices.push_back(baseIndex + idx);
            }

            for (size_t i = 0; i < mesh.indices.size() / 3; ++i)
            {
                result.tags.push_back(info.tag);
            }
        }
    }

    auto terrainView = scene.registry.view<TerrainComponent, PositionComponent>();
    for (auto entity : terrainView)
    {
        auto& terrain = terrainView.get<TerrainComponent>(entity);
        auto& pos = terrainView.get<PositionComponent>(entity);

        if (!terrain.isWalkable)
            continue;

        uint32_t baseIndex = (uint32_t)result.vertices.size();
        glm::vec3 size = terrain.terrainSize;

        int gridRes = (std::max)(2, terrainGridResolution);
        float resStepX = size.x / (float)(gridRes - 1);
        float resStepZ = size.z / (float)(gridRes - 1);

        std::shared_ptr<Texture> tex = nullptr;
        if (resources && !terrain.heightMapName.empty())
        {
            tex = resources->GetTexture(terrain.heightMapName);
        }

        if (tex && !tex->pixelData)
        {
            LOGGER_WARN("NavMeshGenerator") << "Heightmap found but pixelData is NULL for terrain " << (uint32_t)entity;
        }

        for (int z = 0; z < gridRes; ++z)
        {
            for (int x = 0; x < gridRes; ++x)
            {
                float hVal = 0.0f;
                if (tex && tex->pixelData)
                {
                    int texX = (int)((float)x / (float)(gridRes - 1) * (tex->width - 1));
                    int texZ = (int)((float)z / (float)(gridRes - 1) * (tex->height - 1));
                    int idx = (texZ * tex->width + texX) * tex->nrComponents;
                    hVal = (float)tex->pixelData[idx] / 255.0f * terrain.maxHeight;
                }

                result.vertices.push_back(pos.value + glm::vec3(x * resStepX, hVal, z * resStepZ));
            }
        }

        for (int z = 0; z < gridRes - 1; ++z)
        {
            for (int x = 0; x < gridRes - 1; ++x)
            {
                uint32_t i0 = baseIndex + (z * gridRes + x);
                uint32_t i1 = baseIndex + (z * gridRes + (x + 1));
                uint32_t i2 = baseIndex + ((z + 1) * gridRes + (x + 1));
                uint32_t i3 = baseIndex + ((z + 1) * gridRes + x);

                result.indices.push_back(i0);
                result.indices.push_back(i3);
                result.indices.push_back(i2);

                result.indices.push_back(i0);
                result.indices.push_back(i2);
                result.indices.push_back(i1);

                result.tags.push_back("walkable");
                result.tags.push_back("walkable");
            }
        }

        LOGGER_INFO("NavMeshGenerator") << "Generated " << gridRes << "x" << gridRes << " NavMesh grid for terrain "
                                        << (uint32_t)entity;
    }

    LOGGER_INFO("NavMeshGenerator") << "Total geometry gathered: vertices=" << result.vertices.size()
                                    << ", indices=" << result.indices.size();
    return result;
}

void NavMeshGenerator::BuildConnectivity(NavMeshComponent& navMesh)
{
    if (navMesh.triangles.empty())
        return;

    navMesh.nodes.clear();
    navMesh.nodes.reserve(navMesh.triangles.size());
    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i)
    {
        NavMeshNode node;
        node.position = navMesh.triangles[i].center;
        node.triangleIndex = i;
        node.tag = navMesh.triangles[i].tag;
        navMesh.nodes.push_back(node);
    }

    struct Edge
    {
        glm::ivec3 a, b;
        bool operator==(const Edge& other) const
        {
            return a == other.a && b == other.b;
        }
    };
    struct EdgeHash
    {
        std::size_t operator()(const Edge& e) const
        {
            auto hashPoint = [](const glm::ivec3& p) {
                std::size_t h = std::hash<int>{}(p.x);
                h ^= std::hash<int>{}(p.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= std::hash<int>{}(p.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            };
            return hashPoint(e.a) ^ (hashPoint(e.b) << 1);
        }
    };
    auto quantize = [](const glm::vec3& p) {
        constexpr float scale = 1000.0f;
        return glm::ivec3((int)std::round(p.x * scale), (int)std::round(p.y * scale), (int)std::round(p.z * scale));
    };
    std::unordered_map<Edge, std::vector<uint32_t>, EdgeHash> edgeMap;

    for (uint32_t i = 0; i < (uint32_t)navMesh.triangles.size(); ++i)
    {
        const auto& tri = navMesh.triangles[i];
        for (int k = 0; k < 3; ++k)
        {
            glm::ivec3 va = quantize(navMesh.vertices[tri.indices[k]]);
            glm::ivec3 vb = quantize(navMesh.vertices[tri.indices[(k + 1) % 3]]);
            Edge e = (va.x < vb.x || (va.x == vb.x && (va.y < vb.y || (va.y == vb.y && va.z <= vb.z)))) ?
                         Edge{va, vb} :
                         Edge{vb, va};
            edgeMap[e].push_back(i);
        }
    }

    for (auto const& [edge, triIndices] : edgeMap)
    {
        if (triIndices.size() >= 2)
        {
            for (size_t i = 0; i < triIndices.size(); ++i)
            {
                for (size_t j = i + 1; j < triIndices.size(); ++j)
                {
                    uint32_t idx_i = triIndices[i];
                    uint32_t idx_j = triIndices[j];

                    if (std::find(navMesh.nodes[idx_i].neighbors.begin(), navMesh.nodes[idx_i].neighbors.end(),
                                  idx_j) == navMesh.nodes[idx_i].neighbors.end())
                    {
                        navMesh.nodes[idx_i].neighbors.push_back(idx_j);
                    }
                    if (std::find(navMesh.nodes[idx_j].neighbors.begin(), navMesh.nodes[idx_j].neighbors.end(),
                                  idx_i) == navMesh.nodes[idx_j].neighbors.end())
                    {
                        navMesh.nodes[idx_j].neighbors.push_back(idx_i);
                    }
                }
            }
        }
    }
}
