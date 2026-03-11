#pragma once

#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>

class ResourceManager;

class NavMeshGenerator
{
public:
    static void Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources = nullptr, const std::string& walkableTag = "Walkable");

private:
    struct RawMeshData {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
    };

    static RawMeshData GatherWalkableGeometry(Scene& scene, ResourceManager* resources = nullptr, const std::string& walkableTag = "Walkable");
    static void BuildConnectivity(NavMeshComponent& navMesh);
};
