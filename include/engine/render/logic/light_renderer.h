#pragma once

#include <render/type/graphics_types.h>
#include <render/type/render_data.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IBufferManager;
class IGraphicsContext;
class Shader;

class LightRenderer
{
public:
    void Initialize(IGraphicsContext& context);
    void UploadLightData(const RenderSceneData& sceneData, Shader* shader);
    void ConfigureDeferredShader(Shader& shader) const;
    void SetTiledLightCulling(bool enabled, int tileSize);

    int GetDirLightCount() const
    {
        return (int)m_DirLights.size();
    }
    int GetPointLightCount() const
    {
        return (int)m_PointLights.size();
    }
    int GetSpotLightCount() const
    {
        return (int)m_SpotLights.size();
    }

private:
    std::unique_ptr<GPUSSBO> m_DirLightSSBO;
    std::unique_ptr<GPUSSBO> m_PointLightSSBO;
    std::unique_ptr<GPUSSBO> m_SpotLightSSBO;
    std::unique_ptr<GPUSSBO> m_LightTileGridSSBO;
    std::unique_ptr<GPUSSBO> m_LightTileIndicesSSBO;

    std::vector<GPUDirLight> m_DirLights;
    std::vector<GPUPointLight> m_PointLights;
    std::vector<GPUSpotLight> m_SpotLights;

    struct LightTileRange
    {
        uint32_t offset = 0;
        uint32_t count = 0;
    };
    struct LightTileBounds
    {
        int minX = 0;
        int minY = 0;
        int maxX = -1;
        int maxY = -1;
        uint32_t encodedIndex = 0;
    };
    std::vector<LightTileRange> m_LightTileGrid;
    std::vector<uint32_t> m_LightTileIndices;
    std::vector<uint32_t> m_LightTileCursors;
    std::vector<LightTileBounds> m_LightTileBounds;

    std::size_t m_LastCombinedVersion = 0;
    size_t m_LastLightCount = 0;
    std::size_t m_LastTileSpatialHash = 0;
    glm::mat4 m_LastTileView{0.0f};
    glm::mat4 m_LastTileProjection{0.0f};
    int m_LastTileWidth = 0;
    int m_LastTileHeight = 0;
    int m_LightTileCountX = 0;
    int m_LightTileCountY = 0;
    int m_LightTileSize = 16;
    bool m_TiledLightCullingEnabled = true;
    bool m_LightTilesValid = false;

    IGraphicsContext* m_Context = nullptr;

    void UpdateTiledLightData(const RenderSceneData& sceneData, bool spatialLayoutChanged);
};
