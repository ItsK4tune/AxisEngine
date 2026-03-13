#pragma once

#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>

class ResourceManager;

class NavMeshGenerator
{
public:
    static void Generate(Scene& scene, NavMeshComponent& navMesh, ResourceManager* resources = nullptr, 
                        const std::vector<std::string>& walkableTags = { "walkable" }, 
                        const std::vector<std::string>& carveTags = { "obstacle" });

private:
    struct RawMeshData {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::string> tags; // One tag per triangle
    };

    static RawMeshData GatherWalkableGeometry(Scene& scene, ResourceManager* resources = nullptr, 
                                             const std::vector<std::string>& walkableTags = { "walkable" },
                                             const std::vector<std::string>& carveTags = { "obstacle" });
    static void BuildConnectivity(NavMeshComponent& navMesh);
};
