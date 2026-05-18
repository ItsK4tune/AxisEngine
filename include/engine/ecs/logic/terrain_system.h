#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/unit/terrain_component.h>
#include <render/type/graphics_types.h>
#include <resource/unit/shader.h>
#include <entt/entt.hpp>
#include <map>
#include <memory>
#include <vector>

struct TerrainChunk
{
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;
    glm::vec3 minBound, maxBound;
    int lodLevel = -1;
    glm::ivec2 gridPos;
};

class TerrainSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 80;
    }
    std::string GetName() const override
    {
        return "TerrainSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderMain;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override
    {
    }
    void RenderAlphaPass(Scene& scene, int width, int height, float alpha) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void SetDebugNoTexture(bool enable)
    {
        m_DebugNoTexture = enable;
    }
    bool IsDebugNoTexture() const
    {
        return m_DebugNoTexture;
    }

private:
    bool m_Enabled = true;
    bool m_DebugNoTexture = false;

    struct TerrainData
    {
        std::vector<TerrainChunk> chunks;
        std::shared_ptr<Shader> terrainShader;

        std::vector<unsigned int> lodEBOs;
        std::vector<int> lodIndexCounts;
    };

    std::map<entt::entity, std::unique_ptr<TerrainData>> m_TerrainCache;

    void BuildTerrain(entt::entity entity, TerrainComponent& terrain);
    void CreateChunkMesh(TerrainChunk& chunk, const TerrainComponent& terrain, int xOffset, int zOffset);
    void GenerateLODIndices(TerrainData& data, int chunkSize);
    void CleanupTerrainData(TerrainData& data);
};
