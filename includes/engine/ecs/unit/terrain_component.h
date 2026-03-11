#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <physics/interface/i_rigid_body.h>
#include <physics/interface/i_collision_shape.h>

struct TerrainComponent
{
    // Textures
    uint32_t heightMap = 0;
    std::string heightMapName;
    uint32_t splatMap = 0;
    std::vector<uint32_t> diffuseLayers;
    std::vector<uint32_t> normalLayers;

    // Dimensions
    glm::vec3 terrainSize = glm::vec3(512.0f, 50.0f, 512.0f); // Width, MaxHeight, Depth
    float maxHeight = 50.0f;
    int resolution = 1024; // Resolution of the heightmap

    // Chunking
    int chunkSize = 64; // Vertices per side of a chunk
    
    // Rendering parameters
    float textureScale = 1.0f;
    bool castShadows = true;

    // Internal state for TerrainSystem
    bool needsRebuild = true;

    // Physics & Navigation
    bool generatePhysics = false;
    bool isWalkable = false;
    std::shared_ptr<ICollisionShape> collisionShape;
    std::shared_ptr<IRigidBody> physicsBody;
};
