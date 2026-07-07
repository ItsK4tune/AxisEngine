#include "sample_scenario_common.h"
#include <core/logic/logger.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

namespace
{
namespace Perlin
{
inline float fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}
inline float lerp(float t, float a, float b)
{
    return a + t * (b - a);
}
inline float grad(int hash, float x, float y)
{
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

inline int hashCoords(int x, int y)
{
    unsigned int h = x * 374761393 + y * 668265263;
    h = (h ^ (h >> 13)) * 12741261;
    return h ^ (h >> 16);
}

float Noise2D(float x, float y)
{
    int ix = (int)std::floor(x);
    int iy = (int)std::floor(y);
    float fx = x - ix;
    float fy = y - iy;

    float u = fade(fx);
    float v = fade(fy);

    int h00 = hashCoords(ix, iy);
    int h10 = hashCoords(ix + 1, iy);
    int h01 = hashCoords(ix, iy + 1);
    int h11 = hashCoords(ix + 1, iy + 1);

    float n00 = grad(h00, fx, fy);
    float n10 = grad(h10, fx - 1.0f, fy);
    float n01 = grad(h01, fx, fy - 1.0f);
    float n11 = grad(h11, fx - 1.0f, fy - 1.0f);

    float x1 = lerp(u, n00, n10);
    float x2 = lerp(u, n01, n11);

    return (lerp(v, x1, x2) + 1.0f) * 0.5f;  // [0, 1]
}
}  // namespace Perlin

float GetNoise(float x, float z, float frequency, int octaves)
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float freq = frequency * 0.015f;
    float maxValue = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        total += Perlin::Noise2D(x * freq, z * freq) * amplitude;
        maxValue += amplitude;
        freq *= 2.0f;
        amplitude *= 0.5f;
    }
    return total / maxValue;
}
}  // namespace

void SampleState::LoadScene14()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // 1. Generate heightmap and splatmap in memory
    int width = 257;
    int height = 257;
    std::vector<unsigned char> heightPixels(width * height * 3);
    std::vector<unsigned char> splatPixels(width * height * 3);

    std::vector<float> heights(width * height);
    float randomOffsetX = (float)(rand() % 10000);
    float randomOffsetZ = (float)(rand() % 10000);

    float waterLevel = 0.15f;  // Water level relative to maxHeight

    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            float noiseVal =
                GetNoise((float)x + randomOffsetX, (float)z + randomOffsetZ, m_S14NoiseFrequency, m_S14NoiseOctaves);

            // Create winding rivers using 2D Perlin noise
            float riverNoise =
                Perlin::Noise2D((float)x * 0.008f + randomOffsetX + 500.0f, (float)z * 0.008f + randomOffsetZ + 500.0f);
            float riverFactor = std::abs(riverNoise - 0.5f);  // 0.0 at center of river

            // Carve river path
            if (riverFactor < 0.045f)
            {
                float carve = (1.0f - (riverFactor / 0.045f));  // 1.0 at center, 0.0 at edge
                noiseVal = noiseVal * (1.0f - carve) + (waterLevel * 0.5f) * carve;
            }

            heights[z * width + x] = noiseVal;
        }
    }

    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = z * width + x;
            float h = heights[idx];

            unsigned char hByte = (unsigned char)(h * 255.0f);
            heightPixels[idx * 3 + 0] = hByte;  // B
            heightPixels[idx * 3 + 1] = hByte;  // G
            heightPixels[idx * 3 + 2] = hByte;  // R

            float hX = (x < width - 1) ? heights[z * width + (x + 1)] : h;
            float hZ = (z < height - 1) ? heights[(z + 1) * width + x] : h;
            float slope = std::sqrt((hX - h) * (hX - h) + (hZ - h) * (hZ - h)) * 12.0f;
            slope = (std::min)(slope, 1.0f);

            // Slope determines rocks (steeper = more rock)
            float rockWeight = (std::max)(0.0f, (std::min)(1.0f, (slope - 0.15f) / 0.2f));

            // Heights below waterLevel + 0.04f determine Sand (layer 3 / sand_static)
            float sandWeight = 0.0f;
            if (h < waterLevel + 0.04f)
            {
                sandWeight =
                    (std::max)(0.0f, (std::min)(1.0f, ((waterLevel + 0.04f) - h) / 0.04f)) * (1.0f - rockWeight);
            }

            // Lower frequency noise controls clumpy grass vs dirt biomes
            float blendNoise =
                Perlin::Noise2D((float)x * 0.04f + randomOffsetX + 100.0f, (float)z * 0.04f + randomOffsetZ + 100.0f);
            float grassDirtFactor = (std::max)(0.0f, (std::min)(1.0f, (blendNoise - 0.42f) / 0.12f));

            float grassWeight = (1.0f - rockWeight - sandWeight) * grassDirtFactor;
            float dirtWeight = (1.0f - rockWeight - sandWeight) * (1.0f - grassDirtFactor);

            float total = grassWeight + dirtWeight + rockWeight + sandWeight;
            if (total > 0.0f)
            {
                grassWeight /= total;
                dirtWeight /= total;
                rockWeight /= total;
                sandWeight /= total;
            }

            splatPixels[idx * 3 + 0] = (unsigned char)(grassWeight * 255.0f);  // Red
            splatPixels[idx * 3 + 1] = (unsigned char)(dirtWeight * 255.0f);   // Green
            splatPixels[idx * 3 + 2] = (unsigned char)(rockWeight * 255.0f);   // Blue
        }
    }

    // 2. Clear old cached texture resources to force rebuild
    res.UnloadTexture("terrain_height_procedural");
    res.UnloadTexture("terrain_splat_procedural");

    // 3. Create textures in memory, and load static layers from disk
    res.CreateTextureFromData("terrain_height_procedural", heightPixels.data(), width, height, 3, true);
    res.CreateTextureFromData("terrain_splat_procedural", splatPixels.data(), width, height, 3, false);

    res.LoadTexture("terrain_grass_static", "sample/resource/texture/terrain_grass.bmp", false, false);
    res.LoadTexture("terrain_dirt_static", "sample/resource/texture/terrain_dirt.bmp", false, false);
    res.LoadTexture("terrain_rock_static", "sample/resource/texture/terrain_rock.bmp", false, false);
    res.LoadTexture("terrain_snow_static", "sample/resource/texture/terrain_snow.bmp", false, false);

    // 4. Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("TerrainDirLight")
        .WithTransform(glm::vec3(0.0f, 60.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.4f)), glm::vec3(1.0f, 0.95f, 0.88f), 1.6f)
        .Build();

    // 5. Create Terrain Entity
    EntityBuilder(scene, res, "scenario")
        .WithName("ProceduralTerrain")
        .WithTransform(glm::vec3(-m_S14TerrainWidth * 0.5f, 0.0f, -m_S14TerrainLength * 0.5f), glm::vec3(0.0f),
                       glm::vec3(1.0f))
        .WithTerrain(glm::vec3(m_S14TerrainWidth, m_S14TerrainHeight, m_S14TerrainLength), m_S14TerrainHeight, 257, 65,
                     m_S14TextureScale, "terrain_height_procedural", "terrain_splat_procedural",
                     {"terrain_grass_static", "terrain_dirt_static", "terrain_rock_static", "terrain_snow_static"},
                     m_S14GeneratePhysics, true)
        .Build();

    // 6. Spawn Procedural Objects via generic ScatterObjects utility
    // Rules for House
    PlacementRule houseRule;
    houseRule.minHeight = (waterLevel * m_S14TerrainHeight) + 0.5f;
    houseRule.maxHeight = 0.55f * m_S14TerrainHeight;
    houseRule.maxSlope = 0.12f;
    houseRule.waterWeight = 3.5f;
    houseRule.mountainWeight = 0.1f;
    houseRule.plainsWeight = 2.0f;
    houseRule.baseProbability = 3.5f;

    EntityBuilder::ScatterObjects(scene, res, "scenario", "sample/resource/fragment/house.axs", houseRule, heights,
                                  width, height, m_S14TerrainWidth, m_S14TerrainLength, m_S14TerrainHeight, waterLevel,
                                  randomOffsetX, randomOffsetZ, 120, glm::vec3(1.0f));

    // Rules for Tree
    PlacementRule treeRule;
    treeRule.minHeight = (waterLevel * m_S14TerrainHeight) + 0.4f;
    treeRule.maxHeight = 0.65f * m_S14TerrainHeight;
    treeRule.maxSlope = 0.25f;
    treeRule.waterWeight = 2.2f;
    treeRule.mountainWeight = 0.15f;
    treeRule.plainsWeight = 1.8f;
    treeRule.baseProbability = 12.0f;

    EntityBuilder::ScatterObjects(scene, res, "scenario", "sample/resource/fragment/tree.axs", treeRule, heights, width,
                                  height, m_S14TerrainWidth, m_S14TerrainLength, m_S14TerrainHeight, waterLevel,
                                  randomOffsetX, randomOffsetZ, 250, glm::vec3(1.0f));

    // Rules for Rock
    PlacementRule rockRule;
    rockRule.minHeight = (waterLevel * m_S14TerrainHeight) + 0.4f;
    rockRule.maxHeight = 0.95f * m_S14TerrainHeight;
    rockRule.maxSlope = 0.8f;
    rockRule.waterWeight = 1.0f;
    rockRule.mountainWeight = 2.0f;  // Prefer rocky mountains
    rockRule.plainsWeight = 0.5f;
    rockRule.baseProbability = 8.0f;

    EntityBuilder::ScatterObjects(scene, res, "scenario", "sample/resource/fragment/rock.axs", rockRule, heights, width,
                                  height, m_S14TerrainWidth, m_S14TerrainLength, m_S14TerrainHeight, waterLevel,
                                  randomOffsetX, randomOffsetZ, 150, glm::vec3(1.5f));

    // 7. Set camera position to overview the terrain
    for (auto camEntity : GetCameraEntities())
    {
        camEntity.SetPosition(glm::vec3(0.0f, m_S14TerrainHeight + 35.0f, m_S14TerrainLength * 0.65f));
        camEntity.SetRotation(glm::quat(glm::radians(glm::vec3(-22.0f, 0.0f, 0.0f))));
    }

    m_S14SpawnTimer = 0.0f;

    LOGGER_INFO("SampleState") << "Scenario 14 Terrain Scene Loaded.";
}
