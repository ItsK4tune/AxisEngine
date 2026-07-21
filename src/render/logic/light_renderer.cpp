#include <render/logic/light_renderer.h>
#include <render/type/shader_abi.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_target_manager.h>
#include <render/logic/shadow_renderer.h>
#include <render/unit/shadow.h>
#include <resource/unit/shader.h>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <vector>

void LightRenderer::Initialize(IGraphicsContext& context)
{
    m_Context = &context;

    m_DirLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_PointLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_SpotLightSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_LightTileGridSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());
    m_LightTileIndicesSSBO = std::make_unique<GPUSSBO>(context, context.GetBufferManager().CreateBuffer());

    m_DirLights.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    m_PointLights.reserve(Shadow::MAX_POINT_LIGHTS_SHADOW * 2);
    m_SpotLights.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW * 2);
}

void LightRenderer::SetTiledLightCulling(bool enabled, int tileSize)
{
    tileSize = std::clamp(tileSize, 8, 64);
    if (m_TiledLightCullingEnabled == enabled && m_LightTileSize == tileSize)
        return;
    m_TiledLightCullingEnabled = enabled;
    m_LightTileSize = tileSize;
    m_LightTilesValid = false;
}

void LightRenderer::ConfigureDeferredShader(Shader& shader) const
{
    shader.setBool("u_UseTiledLights", m_TiledLightCullingEnabled && m_LightTilesValid);
    shader.setInt("u_LightTileSize", m_LightTileSize);
    shader.setVec2("u_LightTileCount", glm::vec2(static_cast<float>(m_LightTileCountX),
                                                  static_cast<float>(m_LightTileCountY)));
}

void LightRenderer::UpdateTiledLightData(const RenderSceneData& sceneData, bool spatialLayoutChanged)
{
    if (!m_Context || !m_LightTileGridSSBO || !m_LightTileIndicesSSBO)
        return;

    auto& bm = m_Context->GetBufferManager();
    if (!m_TiledLightCullingEnabled)
    {
        m_LightTilesValid = false;
        return;
    }
    // Forward/probe callers only refresh the shared light buffers and do not
    // own the main viewport. Preserve the last valid deferred tile grid.
    if (sceneData.viewportWidth <= 0 || sceneData.viewportHeight <= 0)
        return;

    const bool cameraChanged = m_LastTileWidth != sceneData.viewportWidth ||
                               m_LastTileHeight != sceneData.viewportHeight ||
                               std::memcmp(&m_LastTileView, &sceneData.viewMatrix, sizeof(glm::mat4)) != 0 ||
                               std::memcmp(&m_LastTileProjection, &sceneData.projMatrix, sizeof(glm::mat4)) != 0;
    if (!spatialLayoutChanged && !cameraChanged && m_LightTilesValid)
    {
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::LightTileGridSSBOBinding,
                          m_LightTileGridSSBO->Get());
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::LightTileIndicesSSBOBinding,
                          m_LightTileIndicesSSBO->Get());
        return;
    }

    m_LastTileView = sceneData.viewMatrix;
    m_LastTileProjection = sceneData.projMatrix;
    m_LastTileWidth = sceneData.viewportWidth;
    m_LastTileHeight = sceneData.viewportHeight;
    m_LightTileCountX = (sceneData.viewportWidth + m_LightTileSize - 1) / m_LightTileSize;
    m_LightTileCountY = (sceneData.viewportHeight + m_LightTileSize - 1) / m_LightTileSize;
    const size_t tileCount = static_cast<size_t>(m_LightTileCountX) * static_cast<size_t>(m_LightTileCountY);
    m_LightTileGrid.assign(tileCount, {});
    m_LightTileBounds.clear();
    m_LightTileBounds.reserve(m_PointLights.size() + m_SpotLights.size());

    const float nearPlane = (std::max)(sceneData.nearPlane, 0.0001f);
    const auto appendBounds = [&](const glm::vec3& worldPosition, float radius, uint32_t encodedIndex) {
        radius = (std::max)(radius, 0.0f);
        const glm::vec4 viewPosition4 = sceneData.viewMatrix * glm::vec4(worldPosition, 1.0f);
        const glm::vec3 viewPosition(viewPosition4);
        const float depth = -viewPosition.z;
        if (radius <= 0.0f || depth + radius <= nearPlane)
            return;

        float minNdcX = -1.0f;
        float maxNdcX = 1.0f;
        float minNdcY = -1.0f;
        float maxNdcY = 1.0f;
        if (depth - radius > nearPlane)
        {
            const float radiusSq = radius * radius;
            const float depthSq = depth * depth;
            const auto projectedBounds = [radiusSq, depthSq, depth, radius](float axis, float projectionScale) {
                const float denominator = (std::max)(depthSq - radiusSq, 0.000001f);
                const float root = std::sqrt((std::max)(axis * axis + depthSq - radiusSq, 0.0f));
                const float a = projectionScale * (axis * depth - radius * root) / denominator;
                const float b = projectionScale * (axis * depth + radius * root) / denominator;
                return std::pair<float, float>((std::min)(a, b), (std::max)(a, b));
            };
            const auto xBounds = projectedBounds(viewPosition.x, sceneData.projMatrix[0][0]);
            const auto yBounds = projectedBounds(viewPosition.y, sceneData.projMatrix[1][1]);
            minNdcX = xBounds.first;
            maxNdcX = xBounds.second;
            minNdcY = yBounds.first;
            maxNdcY = yBounds.second;
        }

        if (maxNdcX < -1.0f || minNdcX > 1.0f || maxNdcY < -1.0f || minNdcY > 1.0f)
            return;
        minNdcX = std::clamp(minNdcX, -1.0f, 1.0f);
        maxNdcX = std::clamp(maxNdcX, -1.0f, 1.0f);
        minNdcY = std::clamp(minNdcY, -1.0f, 1.0f);
        maxNdcY = std::clamp(maxNdcY, -1.0f, 1.0f);

        LightTileBounds bounds;
        bounds.minX = std::clamp(static_cast<int>(((minNdcX * 0.5f + 0.5f) * sceneData.viewportWidth) /
                                                  static_cast<float>(m_LightTileSize)),
                                 0, m_LightTileCountX - 1);
        bounds.maxX = std::clamp(static_cast<int>(((maxNdcX * 0.5f + 0.5f) * sceneData.viewportWidth) /
                                                  static_cast<float>(m_LightTileSize)),
                                 0, m_LightTileCountX - 1);
        bounds.minY = std::clamp(static_cast<int>(((minNdcY * 0.5f + 0.5f) * sceneData.viewportHeight) /
                                                  static_cast<float>(m_LightTileSize)),
                                 0, m_LightTileCountY - 1);
        bounds.maxY = std::clamp(static_cast<int>(((maxNdcY * 0.5f + 0.5f) * sceneData.viewportHeight) /
                                                  static_cast<float>(m_LightTileSize)),
                                 0, m_LightTileCountY - 1);
        bounds.encodedIndex = encodedIndex;
        m_LightTileBounds.push_back(bounds);
        for (int y = bounds.minY; y <= bounds.maxY; ++y)
            for (int x = bounds.minX; x <= bounds.maxX; ++x)
                ++m_LightTileGrid[static_cast<size_t>(y) * m_LightTileCountX + x].count;
    };

    for (uint32_t index = 0; index < m_PointLights.size(); ++index)
        appendBounds(m_PointLights[index].position, m_PointLights[index].radius, index);
    constexpr uint32_t SpotLightBit = 0x80000000u;
    for (uint32_t index = 0; index < m_SpotLights.size(); ++index)
    {
        const auto& spot = m_SpotLights[index];
        const float coneRange = (std::max)(spot.radius, 0.0f);
        const float cosAngle = std::clamp(spot.outerCutOff, 0.001f, 1.0f);
        const float cosSq = cosAngle * cosAngle;
        const glm::vec3 direction = glm::length2(spot.direction) > 0.000001f
                                        ? glm::normalize(spot.direction)
                                        : glm::vec3(0.0f, -1.0f, 0.0f);
        float sphereCenterDistance = coneRange;
        float sphereRadius = coneRange * std::sqrt((std::max)(1.0f - cosSq, 0.0f)) / cosAngle;
        if (cosSq >= 0.5f)
        {
            sphereCenterDistance = coneRange / (2.0f * cosSq);
            sphereRadius = sphereCenterDistance;
        }
        appendBounds(spot.position + direction * sphereCenterDistance, sphereRadius, SpotLightBit | index);
    }

    uint32_t offset = 0;
    for (auto& tile : m_LightTileGrid)
    {
        tile.offset = offset;
        offset += tile.count;
    }
    m_LightTileIndices.resize(offset);
    m_LightTileCursors.resize(tileCount);
    for (size_t index = 0; index < tileCount; ++index)
        m_LightTileCursors[index] = m_LightTileGrid[index].offset;
    for (const auto& bounds : m_LightTileBounds)
    {
        for (int y = bounds.minY; y <= bounds.maxY; ++y)
        {
            for (int x = bounds.minX; x <= bounds.maxX; ++x)
            {
                const size_t tileIndex = static_cast<size_t>(y) * m_LightTileCountX + x;
                m_LightTileIndices[m_LightTileCursors[tileIndex]++] = bounds.encodedIndex;
            }
        }
    }

    static const LightTileRange dummyRange{};
    static const uint32_t dummyIndex = 0;
    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_LightTileGridSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer,
                  (std::max)(m_LightTileGrid.size(), size_t{1}) * sizeof(LightTileRange),
                  m_LightTileGrid.empty() ? &dummyRange : m_LightTileGrid.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::LightTileGridSSBOBinding,
                      m_LightTileGridSSBO->Get());
    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_LightTileIndicesSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer,
                  (std::max)(m_LightTileIndices.size(), size_t{1}) * sizeof(uint32_t),
                  m_LightTileIndices.empty() ? &dummyIndex : m_LightTileIndices.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::LightTileIndicesSSBOBinding,
                      m_LightTileIndicesSSBO->Get());
    m_LightTilesValid = true;
}

void LightRenderer::UploadLightData(const RenderSceneData& sceneData, Shader* shader)
{
    if (!m_Context)
        return;
    auto& bm = m_Context->GetBufferManager();

    std::size_t currentCombinedVersion = 0;
    std::size_t currentTileSpatialHash = 0;
    auto hashCombine = [&currentCombinedVersion](std::size_t value) {
        currentCombinedVersion ^=
            value + 0x9e3779b97f4a7c15ULL + (currentCombinedVersion << 6) + (currentCombinedVersion >> 2);
    };
    auto hashFloat = [](float value) { return std::hash<float>{}(value); };
    auto hashVec3 = [&](const glm::vec3& value) {
        hashCombine(hashFloat(value.x));
        hashCombine(hashFloat(value.y));
        hashCombine(hashFloat(value.z));
    };
    auto hashTileValue = [&currentTileSpatialHash](std::size_t value) {
        currentTileSpatialHash ^=
            value + 0x9e3779b97f4a7c15ULL + (currentTileSpatialHash << 6) + (currentTileSpatialHash >> 2);
    };
    auto hashTileFloat = [&hashTileValue](float value) { hashTileValue(std::hash<float>{}(value)); };
    auto hashTileVec3 = [&hashTileFloat](const glm::vec3& value) {
        hashTileFloat(value.x);
        hashTileFloat(value.y);
        hashTileFloat(value.z);
    };
    int shadowMode = 0;
    bool shadowsEnabled = false;
    if (auto* shadowSys = ServiceLocator::Instance().Resolve<IShadowService>())
    {
        auto& renderer = shadowSys->GetRenderer();
        shadowMode = renderer.GetShadowMode();
        shadowsEnabled = renderer.IsShadowsEnabled();
    }

    hashCombine(std::hash<int>{}(shadowMode));
    hashCombine(std::hash<bool>{}(shadowsEnabled));

    for (const auto& light : sceneData.GetLights())
    {
        hashCombine(std::hash<int>{}(static_cast<int>(light.type)));
        hashCombine(light.version);
        hashVec3(light.position);
        hashVec3(light.direction);
        hashVec3(light.color);
        hashVec3(light.ambient);
        hashVec3(light.diffuse);
        hashVec3(light.specular);
        hashCombine(hashFloat(light.intensity));
        hashCombine(hashFloat(light.constant));
        hashCombine(hashFloat(light.linear));
        hashCombine(hashFloat(light.quadratic));
        hashCombine(hashFloat(light.range));
        hashCombine(hashFloat(light.innerCutoff));
        hashCombine(hashFloat(light.outerCutoff));
        hashCombine(std::hash<bool>{}(light.castShadows));
        hashCombine(std::hash<int>{}(light.shadowMapIndex));
        if (light.type != RenderLightType::Directional)
        {
            hashTileValue(std::hash<int>{}(static_cast<int>(light.type)));
            hashTileVec3(light.position);
            hashTileFloat(light.range);
            if (light.type == RenderLightType::Spot)
            {
                hashTileVec3(light.direction);
                hashTileFloat(light.outerCutoff);
            }
        }
    }
    hashTileValue(sceneData.GetLights().size());
    std::size_t currentLightCount = sceneData.GetLights().size();
    const bool lightsChanged =
        currentCombinedVersion != m_LastCombinedVersion || currentLightCount != m_LastLightCount;

    if (lightsChanged)
    {
        m_DirLights.clear();
        m_PointLights.clear();
        m_SpotLights.clear();

        for (const auto& light : sceneData.GetLights())
        {
            int shadowIdx = (light.castShadows && shadowsEnabled && shadowMode != 0) ? light.shadowMapIndex : -1;

            if (light.type == RenderLightType::Directional)
                m_DirLights.push_back({light.direction, (float)shadowIdx, light.color, light.intensity, light.ambient,
                                       0.0f, light.diffuse, 0.0f, light.specular, 0.0f});
            else if (light.type == RenderLightType::Point)
                m_PointLights.push_back({light.position, (float)shadowIdx, light.color, light.intensity, light.constant,
                                         light.linear, light.quadratic, light.range, light.ambient, 0.0f, light.diffuse,
                                         0.0f, light.specular, 0.0f});
            else if (light.type == RenderLightType::Spot)
                m_SpotLights.push_back({light.position, 0.0f, light.direction, (float)shadowIdx, light.color,
                                        light.intensity, light.innerCutoff, light.outerCutoff, light.constant,
                                        light.linear, light.quadratic, light.range, 0.0f, 0.0f, light.ambient, 0.0f,
                                        light.diffuse, 0.0f, light.specular, 0.0f});
        }
    }

    const bool tileSpatialLayoutChanged = currentTileSpatialHash != m_LastTileSpatialHash;
    m_LastTileSpatialHash = currentTileSpatialHash;
    UpdateTiledLightData(sceneData, tileSpatialLayoutChanged);

    auto* shadowSys = ServiceLocator::Instance().Resolve<IShadowService>();
    auto* rs = ServiceLocator::Instance().Resolve<IRenderService>();
    if (shadowSys && rs)
    {
        auto& sr = shadowSys->GetRenderer();
        GPUGlobalLightData data;
        std::memcpy(data.lightSpaceMatricesDir, sr.GetLightSpaceMatrices(),
                    sizeof(glm::mat4) * Shadow::MAX_DIR_LIGHTS_SHADOW);
        std::memcpy(data.lightSpaceMatricesSpot, sr.GetLightSpaceMatricesSpot(),
                    sizeof(glm::mat4) * Shadow::MAX_SPOT_LIGHTS_SHADOW);
        data.numDirLights = (int)m_DirLights.size();
        data.nrPointLights = (int)m_PointLights.size();
        data.nrSpotLights = (int)m_SpotLights.size();
        data.u_ReceiveShadow = (sr.IsShadowsEnabled() && sr.GetShadowMode() != 0) ? 1 : 0;
        data.farPlanePoint = sr.GetFarPlanePoint();
        data.farPlaneSpot = sr.GetFarPlaneSpot();
        data.pad0 = 0.0f;
        data.pad1 = 0.0f;

        rs->UpdateGlobalLightData(data);
    }

    if (!lightsChanged)
    {
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::DirectionalLightSSBOBinding,
                          m_DirLightSSBO->Get());
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::PointLightSSBOBinding, m_PointLightSSBO->Get());
        bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::SpotLightSSBOBinding, m_SpotLightSSBO->Get());
        return;
    }

    m_LastCombinedVersion = currentCombinedVersion;
    m_LastLightCount = currentLightCount;

    auto safeSize = [](size_t count, size_t unitSize) { return (std::max)(count, (size_t)1) * unitSize; };

    static const GPUDirLight dummyDir{};
    static const GPUPointLight dummyPoint{};
    static const GPUSpotLight dummySpot{};

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_DirLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_DirLights.size(), sizeof(GPUDirLight)),
                  m_DirLights.empty() ? &dummyDir : m_DirLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::DirectionalLightSSBOBinding, m_DirLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_PointLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_PointLights.size(), sizeof(GPUPointLight)),
                  m_PointLights.empty() ? &dummyPoint : m_PointLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::PointLightSSBOBinding, m_PointLightSSBO->Get());

    bm.BindBuffer(BufferType::ShaderStorageBuffer, m_SpotLightSSBO->Get());
    bm.BufferData(BufferType::ShaderStorageBuffer, safeSize(m_SpotLights.size(), sizeof(GPUSpotLight)),
                  m_SpotLights.empty() ? &dummySpot : m_SpotLights.data(), BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::ShaderStorageBuffer, ShaderABI::SpotLightSSBOBinding, m_SpotLightSSBO->Get());
}
