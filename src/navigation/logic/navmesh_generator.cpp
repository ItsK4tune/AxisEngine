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
#include <limits>
#include <map>
#include <unordered_map>

namespace
{
bool ContainsXZ(const glm::vec3& point, const glm::vec3& minimum, const glm::vec3& maximum)
{
    return point.x >= minimum.x && point.x <= maximum.x && point.z >= minimum.z && point.z <= maximum.z;
}

bool IsCarved(const NavMeshTriangle& triangle, const std::vector<NavMeshGenerator::ObstacleBounds>& obstacles,
              float heightPadding)
{
    for (const auto& obstacle : obstacles)
    {
        if (ContainsXZ(triangle.center, obstacle.min, obstacle.max) &&
            triangle.center.y >= obstacle.min.y - heightPadding &&
            triangle.center.y <= obstacle.max.y + heightPadding)
            return true;
    }
    return false;
}
}  // namespace

std::vector<NavMeshGenerator::ObstacleBounds> NavMeshGenerator::CollectObstacleBounds(
    Scene& scene, const NavMeshComponent& navMesh, const std::vector<std::string>& carveTags)
{
    std::vector<ObstacleBounds> obstacles;
    auto infoView = scene.View<InfoComponent, WorldTransformComponent, MeshRendererComponent>();
    obstacles.reserve(infoView.size_hint());
    for (const entt::entity entity : infoView)
    {
        const auto& info = infoView.get<InfoComponent>(entity);
        if (std::find(carveTags.begin(), carveTags.end(), info.tag) == carveTags.end())
            continue;

        const auto& transform = infoView.get<WorldTransformComponent>(entity);
        const auto& renderer = infoView.get<MeshRendererComponent>(entity);
        glm::vec3 localMin(-0.5f);
        glm::vec3 localMax(0.5f);
        if (renderer.model)
        {
            localMin = renderer.model->aabb.minBound;
            localMax = renderer.model->aabb.maxBound;
        }

        // Transform all corners: transforming only min/max is incorrect for rotations.
        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());
        for (int corner = 0; corner < 8; ++corner)
        {
            const glm::vec3 local((corner & 1) ? localMax.x : localMin.x,
                                  (corner & 2) ? localMax.y : localMin.y,
                                  (corner & 4) ? localMax.z : localMin.z);
            const glm::vec3 world = glm::vec3(transform.worldMatrix * glm::vec4(local, 1.0f));
            worldMin = glm::min(worldMin, world);
            worldMax = glm::max(worldMax, world);
        }
        worldMin.x -= navMesh.carveAgentRadius;
        worldMin.z -= navMesh.carveAgentRadius;
        worldMax.x += navMesh.carveAgentRadius;
        worldMax.z += navMesh.carveAgentRadius;
        obstacles.push_back({entity, worldMin, worldMax});
    }
    return obstacles;
}

void NavMeshGenerator::Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources,
                                const std::vector<std::string>& walkableTags, const std::vector<std::string>& carveTags,
                                std::vector<NavMeshTriangle>* uncarvedTriangles)
{
    navMesh.needsRebuild = false;

    navMesh.vertices.clear();
    navMesh.triangles.clear();
    navMesh.nodes.clear();
    if (uncarvedTriangles)
        uncarvedTriangles->clear();

    RawMeshData raw = GatherWalkableGeometry(scene, resources, walkableTags, carveTags,
                                             navMesh.terrainGridResolution);
    if (raw.vertices.empty())
    {
        LOGGER_WARN("NavMeshGenerator") << "No walkable geometry found! NavMesh will be empty.";
        ++navMesh.revision;
        return;
    }

    const auto obstacles = CollectObstacleBounds(scene, navMesh, carveTags);

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
            if (uncarvedTriangles)
                uncarvedTriangles->push_back(tri);
            if (!IsCarved(tri, obstacles, navMesh.carveHeightPadding))
                navMesh.triangles.push_back(tri);
        }
    }

    BuildConnectivity(navMesh);
    ++navMesh.revision;
}

void NavMeshGenerator::RebuildRegion(Scene& scene, NavMeshComponent& navMesh,
                                     const std::vector<NavMeshTriangle>& uncarvedTriangles,
                                     const glm::vec3& dirtyMin, const glm::vec3& dirtyMax,
                                     const std::vector<std::string>& carveTags)
{
    if (uncarvedTriangles.empty())
        return;
    const auto obstacles = CollectObstacleBounds(scene, navMesh, carveTags);
    std::erase_if(navMesh.triangles,
                  [&](const NavMeshTriangle& triangle) { return ContainsXZ(triangle.center, dirtyMin, dirtyMax); });
    for (const auto& triangle : uncarvedTriangles)
    {
        if (ContainsXZ(triangle.center, dirtyMin, dirtyMax) &&
            !IsCarved(triangle, obstacles, navMesh.carveHeightPadding))
            navMesh.triangles.push_back(triangle);
    }
    std::sort(navMesh.triangles.begin(), navMesh.triangles.end(), [](const auto& left, const auto& right) {
        if (left.indices[0] != right.indices[0])
            return left.indices[0] < right.indices[0];
        if (left.indices[1] != right.indices[1])
            return left.indices[1] < right.indices[1];
        return left.indices[2] < right.indices[2];
    });
    BuildConnectivity(navMesh);
    ++navMesh.revision;
}

NavMeshGenerator::RawMeshData NavMeshGenerator::GatherWalkableGeometry(Scene& scene, ResourceManager* resources,
                                                                       const std::vector<std::string>& walkableTags,
                                                                       const std::vector<std::string>& carveTags,
                                                                       int terrainGridResolution)
{
    RawMeshData result;

    LOGGER_INFO("NavMeshGenerator") << "Gathering walkable geometry for " << walkableTags.size() << " tags.";

    auto meshView = scene.View<InfoComponent, MeshRendererComponent, WorldTransformComponent>();
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
                const glm::vec3 pos = mesh.GetPosition(vIdx);
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

    auto terrainView = scene.View<TerrainComponent, PositionComponent>();
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
