#pragma once

#include <ecs/interface/i_system.h>
#include <entt/entt.hpp>
#include <ecs/unit/terrain_component.h>
#include <render/type/graphics_types.h>
#include <render/logic/shader.h>
#include <vector>
#include <map>
#include <memory>

struct TerrainChunk {
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;
    glm::vec3 minBound, maxBound;
    int lodLevel = -1;
    glm::ivec2 gridPos;
};

class TerrainSystem : public ISystem {
public:
    void Initialize(EngineContext ctx) override;
    void Shutdown() override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 25; }
    std::string GetName() const override { return "TerrainSystem"; }

    void Update(Scene &scene, float dt) override;
    void Render(Scene &scene) override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;

    struct TerrainData {
        std::vector<TerrainChunk> chunks;
        std::shared_ptr<Shader> terrainShader;
        // Shared index buffers for LODs
        std::vector<unsigned int> lodEBOs;
        std::vector<int> lodIndexCounts;
    };

    std::map<entt::entity, std::unique_ptr<TerrainData>> m_TerrainCache;

    void BuildTerrain(entt::entity entity, TerrainComponent& terrain);
    void CreateChunkMesh(TerrainChunk& chunk, const TerrainComponent& terrain, int xOffset, int zOffset);
    void GenerateLODIndices(TerrainData& data, int chunkSize);
    void CleanupTerrainData(TerrainData& data);
};
