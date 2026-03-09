#pragma once

#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>

class NavMeshGenerator
{
public:
    static void Generate(Scene& scene, NavMeshComponent& navMesh);

private:
    struct RawMeshData {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
    };

    static RawMeshData GatherWalkableGeometry(Scene& scene);
    static void BuildConnectivity(NavMeshComponent& navMesh);
};
