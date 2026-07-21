#pragma once

#include <navigation/unit/navmesh_component.h>
#include <scene/logic/scene.h>

class ResourceManager;

class NavMeshGenerator
{
public:
    struct ObstacleBounds
    {
        entt::entity entity = entt::null;
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };

    static void Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources = nullptr,
                         const std::vector<std::string>& walkableTags = {"walkable"},
                         const std::vector<std::string>& carveTags = {"obstacle"},
                         std::vector<NavMeshTriangle>* uncarvedTriangles = nullptr);
    static std::vector<ObstacleBounds> CollectObstacleBounds(
        Scene& scene, const NavMeshComponent& navMesh,
        const std::vector<std::string>& carveTags = {"obstacle"});
    static void RebuildRegion(Scene& scene, NavMeshComponent& navMesh,
                              const std::vector<NavMeshTriangle>& uncarvedTriangles,
                              const glm::vec3& dirtyMin, const glm::vec3& dirtyMax,
                              const std::vector<std::string>& carveTags = {"obstacle"});

private:
    struct RawMeshData
    {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::string> tags;
    };

    static RawMeshData GatherWalkableGeometry(Scene& scene, ResourceManager* resources = nullptr,
                                              const std::vector<std::string>& walkableTags = {"walkable"},
                                              const std::vector<std::string>& carveTags = {"obstacle"},
                                              int terrainGridResolution = 64);
    static void BuildConnectivity(NavMeshComponent& navMesh);
};
