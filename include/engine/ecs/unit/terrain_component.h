#pragma once

#include <physics/interface/i_collision_shape.h>
#include <physics/interface/i_rigid_body.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct TerrainComponent
{
    uint32_t heightMap = 0;
    std::string heightMapName;
    uint32_t splatMap = 0;
    std::vector<uint32_t> diffuseLayers;
    std::vector<uint32_t> normalLayers;

    glm::vec3 terrainSize = glm::vec3(512.0f, 50.0f, 512.0f);
    float maxHeight = 50.0f;
    int resolution = 1024;

    int chunkSize = 64;

    float textureScale = 1.0f;
    bool castShadows = true;

    bool needsRebuild = true;

    bool generatePhysics = false;
    bool isWalkable = false;
    std::shared_ptr<ICollisionShape> collisionShape;
    std::shared_ptr<IRigidBody> physicsBody;
    std::string customShader;
};
