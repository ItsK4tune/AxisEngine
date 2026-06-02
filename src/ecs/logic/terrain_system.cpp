#include <ecs/logic/terrain_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/terrain_component.h>
#include <physics/interface/i_physics_world.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/frustum_culler.h>
#include <render/unit/frustum.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/shader.h>
#include <scene/logic/scene.h>
#include <glm/gtc/matrix_transform.hpp>

REGISTER_SYSTEM(TerrainSystem)

void TerrainSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<TerrainSystem>(this);
    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All))
        {
        }
    });
    EventManager::Instance().Subscribe<DebugNoTextureChangedEvent>(
        [this](const DebugNoTextureChangedEvent& e) { SetDebugNoTexture(e.enabled); });
}

void TerrainSystem::Shutdown()
{
    if (m_LastScene)
    {
        m_LastScene->registry.on_destroy<TerrainComponent>().disconnect<&TerrainSystem::OnTerrainDestroyed>(this);
        m_LastScene = nullptr;
    }

    for (auto& pair : m_TerrainCache)
    {
        CleanupTerrainData(*pair.second);
    }
    m_TerrainCache.clear();
}

void TerrainSystem::Update(Scene& scene, float dt)
{
    if (m_LastScene != &scene)
    {
        if (m_LastScene)
        {
            m_LastScene->registry.on_destroy<TerrainComponent>().disconnect<&TerrainSystem::OnTerrainDestroyed>(this);
        }
        scene.registry.on_destroy<TerrainComponent>().connect<&TerrainSystem::OnTerrainDestroyed>(this);
        m_LastScene = &scene;
    }

    if (!m_Enabled)
        return;

    auto view = scene.registry.view<TerrainComponent>();
    for (auto entity : view)
    {
        auto& terrain = view.get<TerrainComponent>(entity);
        if (terrain.needsRebuild)
        {
            LOGGER_INFO("TerrainSystem") << "Rebuilding terrain for entity " << (uint32_t)entity;
            BuildTerrain(entity, terrain);
        }
    }
}

void TerrainSystem::Render(Scene& scene)
{
    if (!m_Enabled)
        return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null)
        return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    auto& sl = ServiceLocator::Instance();
    auto* gc_ptr = sl.Resolve<IGraphicsContext>();
    auto* resources = sl.Resolve<ResourceManager>();
    if (!gc_ptr || !resources)
        return;

    auto& gc = *gc_ptr;
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();
    auto& rtm = gc.GetRenderTargetManager();

    FrustumCuller culler;
    culler.BuildFrustum(cam.projectionMatrix * cam.viewMatrix);

    auto* geoSys = sl.Resolve<IGeometryService>();

    auto view = scene.registry.view<TerrainComponent, PositionComponent>();
    for (auto entity : view)
    {
        auto& terrain = view.get<TerrainComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);

        if (auto* info = scene.registry.try_get<InfoComponent>(entity); info && !info->isActive)
            continue;

        auto it = m_TerrainCache.find(entity);
        if (it == m_TerrainCache.end())
        {
            continue;
        }

        TerrainData& data = *it->second;
        Shader* actShader = data.terrainShader.get();
        if (!terrain.customShader.empty())
        {
            if (auto custom = resources->GetShader(terrain.customShader))
            {
                actShader = custom.get();
            }
        }

        if (geoSys && geoSys->IsDeferredRenderingEnabled())
        {
            auto gbufShader = resources->GetShader("terrain_gbuffer");
            if (!terrain.customShader.empty())
            {
                if (auto custom = resources->GetShader(terrain.customShader + "_gbuffer"))
                {
                    gbufShader = custom;
                }
            }

            if (gbufShader)
                actShader = gbufShader.get();
            geoSys->BindGBufferForWriting();
        }

        if (!actShader)
            continue;

        actShader->use();
        actShader->setMat4("view", cam.viewMatrix);
        actShader->setMat4("projection", cam.projectionMatrix);
        actShader->setVec3("camPos", camPos);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value);
        actShader->setMat4("model", model);
        actShader->setMat4("u_Model", model);
        actShader->setUInt("entityID", static_cast<unsigned int>(entity));
        actShader->setUInt("u_EntityID", static_cast<unsigned int>(entity));

        actShader->setFloat("maxHeight", terrain.maxHeight);
        actShader->setVec2("terrainSize", glm::vec2(terrain.terrainSize.x, terrain.terrainSize.z));
        actShader->setFloat("textureScale", terrain.textureScale);
        actShader->setBool("debug_noTexture", m_DebugNoTexture);

        tm.ActiveTexture(TextureUnit::Texture26);
        tm.BindTexture(TextureType::Texture2D, terrain.heightMap);
        actShader->setInt("heightMap", 26);

        tm.ActiveTexture(TextureUnit::Texture27);
        tm.BindTexture(TextureType::Texture2D, terrain.splatMap);
        actShader->setInt("splatMap", 27);

        for (size_t i = 0; i < terrain.diffuseLayers.size() && i < 4; ++i)
        {
            tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture28) + i));
            tm.BindTexture(TextureType::Texture2D, terrain.diffuseLayers[i]);
            std::string name = "textureLayer" + std::to_string(i);
            actShader->setInt(name.c_str(), 28 + (int)i);
        }

        for (auto& chunk : data.chunks)
        {
            if (!culler.IsVisible(chunk.minBound + pos.value, chunk.maxBound + pos.value))
            {
                continue;
            }

            float dist = glm::distance(camPos, (chunk.minBound + chunk.maxBound) * 0.5f + pos.value);
            int lod = 0;
            if (dist > 300.0f)
                lod = 3;
            else if (dist > 150.0f)
                lod = 2;
            else if (dist > 75.0f)
                lod = 1;

            if (lod >= (int)data.lodEBOs.size())
                continue;

            bm.BindVertexArray(chunk.VAO);
            bm.BindBuffer(BufferType::ElementArrayBuffer, data.lodEBOs[lod]);
            dc.DrawElements(Primitive::Triangles, data.lodIndexCounts[lod], DataType::UnsignedInt, nullptr);
            auto* rs = sl.Resolve<IRenderService>();
            if (rs)
                rs->AddRenderedCount(1);
        }

        if (geoSys && geoSys->IsDeferredRenderingEnabled())
        {
            geoSys->UnbindGBuffer();
            auto* rs = sl.Resolve<IRenderService>();
            if (rs)
            {
                rtm.BindFramebuffer(FramebufferTarget::Framebuffer, rs->GetMainFBO());
            }
        }
    }
}

void TerrainSystem::RenderShadowPass(Scene& scene, Shader& shader, const Frustum* lightFrustum,
                                     const glm::vec3& cullingOrigin, float distanceCullingSq)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* gc_ptr = sl.Resolve<IGraphicsContext>();
    if (!gc_ptr)
        return;

    auto& bm = gc_ptr->GetBufferManager();
    auto& dc = gc_ptr->GetDrawContext();

    shader.use();
    shader.setBool("u_HasAnimation", false);

    auto view = scene.registry.view<TerrainComponent, PositionComponent>();
    for (auto entity : view)
    {
        auto& terrain = view.get<TerrainComponent>(entity);
        if (!terrain.castShadows)
            continue;
        if (auto* info = scene.registry.try_get<InfoComponent>(entity); info && !info->isActive)
            continue;

        auto it = m_TerrainCache.find(entity);
        if (it == m_TerrainCache.end() || !it->second)
            continue;

        TerrainData& data = *it->second;
        if (data.lodEBOs.empty() || data.lodIndexCounts.empty())
            continue;

        auto& pos = view.get<PositionComponent>(entity);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value);
        shader.setMat4("u_Model", model);

        for (const auto& chunk : data.chunks)
        {
            glm::vec3 minBound = chunk.minBound + pos.value;
            glm::vec3 maxBound = chunk.maxBound + pos.value;

            if (lightFrustum && !lightFrustum->IsBoxVisible(minBound, maxBound))
                continue;

            if (distanceCullingSq > 0.0f)
            {
                glm::vec3 center = (minBound + maxBound) * 0.5f;
                glm::vec3 delta = center - cullingOrigin;
                if (glm::dot(delta, delta) > distanceCullingSq)
                    continue;
            }

            bm.BindVertexArray(chunk.VAO);
            bm.BindBuffer(BufferType::ElementArrayBuffer, data.lodEBOs[0]);
            dc.DrawElements(Primitive::Triangles, data.lodIndexCounts[0], DataType::UnsignedInt, nullptr);
        }
    }

    bm.BindVertexArray(0);
}

void TerrainSystem::BuildTerrain(entt::entity entity, TerrainComponent& terrain)
{
    auto& sl = ServiceLocator::Instance();
    auto* resources = sl.Resolve<ResourceManager>();
    if (!resources)
        return;

    auto data = std::make_unique<TerrainData>();

    data->terrainShader = resources->GetShader("terrain");

    if (!data->terrainShader)
    {
        LOGGER_ERROR("TerrainSystem") << "Cannot build terrain: shader failed to load.";
        return;
    }

    bool needsRetry = false;
    std::vector<float> heights;
    int mapWidth = 0;
    int mapHeight = 0;

    if (!terrain.heightMapName.empty())
    {
        auto tex = resources->GetTexture(terrain.heightMapName);
        if (tex && tex->pixelData)
        {
            mapWidth = tex->width;
            mapHeight = tex->height;
            heights.reserve(mapWidth * mapHeight);
            for (int i = 0; i < mapWidth * mapHeight; ++i)
            {
                float hValue = (float)tex->pixelData[i * tex->nrComponents] / 255.0f;
                heights.push_back(hValue * terrain.maxHeight);
            }
        }
        else
        {
            needsRetry = true;
        }
    }

    if (needsRetry)
    {
        return;
    }

    GenerateLODIndices(*data, terrain.chunkSize);

    int numChunksX = (terrain.resolution - 1) / (terrain.chunkSize - 1);
    int numChunksZ = (terrain.resolution - 1) / (terrain.chunkSize - 1);

    for (int z = 0; z < numChunksZ; ++z)
    {
        for (int x = 0; x < numChunksX; ++x)
        {
            TerrainChunk chunk;
            chunk.gridPos = glm::ivec2(x, z);
            CreateChunkMesh(chunk, terrain, x * (terrain.chunkSize - 1), z * (terrain.chunkSize - 1), heights, mapWidth, mapHeight);
            data->chunks.push_back(chunk);
        }
    }

    auto cacheIt = m_TerrainCache.find(entity);
    if (cacheIt != m_TerrainCache.end() && cacheIt->second)
    {
        CleanupTerrainData(*cacheIt->second);
    }

    m_TerrainCache[entity] = std::move(data);

    LOGGER_INFO("TerrainSystem") << "Checking physics generation for entity " << (uint32_t)entity
                                 << ": generate=" << terrain.generatePhysics << ", mapName=" << terrain.heightMapName;

    glm::vec3 posVal(0.0f);
    if (m_LastScene)
    {
        if (auto* posComp = m_LastScene->registry.try_get<PositionComponent>(entity))
        {
            posVal = posComp->value;
        }
    }
    LOGGER_INFO("TerrainSystem") << "BuildTerrain posVal for entity " << (uint32_t)entity << " is (" << posVal.x << ", " << posVal.y << ", " << posVal.z << ")";

    auto* gc = sl.Resolve<IGraphicsContext>();
    if (gc)
    {
        auto& tm = gc->GetTextureManager();
        tm.ActiveTexture(TextureUnit::Texture0);
        if (terrain.heightMap != 0)
        {
            tm.BindTexture(TextureType::Texture2D, terrain.heightMap);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Linear);
        }
        if (terrain.splatMap != 0)
        {
            tm.BindTexture(TextureType::Texture2D, terrain.splatMap);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, (int)TextureWrap::ClampToEdge);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Linear);
        }
    }

    auto physics_ptr = sl.Resolve<IPhysicsWorld>();
    if (terrain.generatePhysics && physics_ptr && !heights.empty())
    {
        if (terrain.physicsBody)
        {
            physics_ptr->RemoveRigidBody(terrain.physicsBody.get());
            terrain.physicsBody.reset();
        }

        if (terrain.collisionShape)
        {
            terrain.collisionShape.reset();
        }

        terrain.collisionShape =
            physics_ptr->CreateHeightfieldShape(heights, mapWidth, mapHeight, 0.0f, terrain.maxHeight);

        if (terrain.collisionShape)
        {
            float scaleX = terrain.terrainSize.x / (float)(mapWidth > 1 ? mapWidth - 1 : 1);
            float scaleZ = terrain.terrainSize.z / (float)(mapHeight > 1 ? mapHeight - 1 : 1);
            terrain.collisionShape->SetLocalScaling(glm::vec3(scaleX, 1.0f, scaleZ));

            glm::vec3 centerPos = posVal + glm::vec3(terrain.terrainSize.x * 0.5f, terrain.maxHeight * 0.5f, terrain.terrainSize.z * 0.5f);

            terrain.physicsBody =
                physics_ptr->CreateRigidBody(0.0f, centerPos, glm::quat(1, 0, 0, 0), terrain.collisionShape);

            if (terrain.physicsBody)
            {
                terrain.physicsBody->SetUserPointer((void*)((uintptr_t)entity + 1));
                physics_ptr->AddRigidBody(terrain.physicsBody.get());
            }
        }
    }

    if (!needsRetry)
    {
        terrain.needsRebuild = false;
    }
}

void TerrainSystem::CreateChunkMesh(TerrainChunk& chunk, const TerrainComponent& terrain, int xOffset, int zOffset, const std::vector<float>& heights, int mapWidth, int mapHeight)
{
    auto* gc_ptr = ServiceLocator::Instance().Resolve<IGraphicsContext>();
    if (!gc_ptr)
        return;
    auto& bm = gc_ptr->GetBufferManager();

    std::vector<StaticVertex> vertices;
    int size = terrain.chunkSize;
    float stepX = terrain.terrainSize.x / (float)(terrain.resolution - 1);
    float stepZ = terrain.terrainSize.z / (float)(terrain.resolution - 1);

    chunk.minBound = glm::vec3(xOffset * stepX, 0.0f, zOffset * stepZ);
    chunk.maxBound = glm::vec3((xOffset + size - 1) * stepX, terrain.maxHeight, (zOffset + size - 1) * stepZ);

    auto getHeight = [&](int gx, int gz) -> float {
        int tx = (int)std::round((float)gx / (float)(terrain.resolution - 1) * (float)(mapWidth - 1));
        int tz = (int)std::round((float)gz / (float)(terrain.resolution - 1) * (float)(mapHeight - 1));
        tx = std::max(0, std::min(tx, mapWidth - 1));
        tz = std::max(0, std::min(tz, mapHeight - 1));
        if (heights.empty() || tx >= mapWidth || tz >= mapHeight)
            return 0.0f;
        return heights[tz * mapWidth + tx];
    };

    for (int z = 0; z < size; ++z)
    {
        for (int x = 0; x < size; ++x)
        {
            StaticVertex v;
            int gridX = xOffset + x;
            int gridZ = zOffset + z;
            float worldX = gridX * stepX;
            float worldZ = gridZ * stepZ;

            float h = getHeight(gridX, gridZ);
            v.Position = glm::vec3(worldX, h, worldZ);

            // Compute normal using central difference
            float hL = getHeight(gridX - 1, gridZ);
            float hR = getHeight(gridX + 1, gridZ);
            float hD = getHeight(gridX, gridZ - 1);
            float hU = getHeight(gridX, gridZ + 1);

            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * stepX, hD - hU));
            v.Normal = normal;

            v.TexCoords = glm::vec2((float)gridX / (float)(terrain.resolution - 1),
                                    (float)gridZ / (float)(terrain.resolution - 1));

            vertices.push_back(v);
        }
    }

    chunk.VAO = bm.CreateVertexArray();
    chunk.VBO = bm.CreateBuffer();

    bm.BindVertexArray(chunk.VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, chunk.VBO);
    bm.BufferData(BufferType::ArrayBuffer, vertices.size() * sizeof(StaticVertex), vertices.data(),
                  BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Position));
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Normal));
    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, sizeof(StaticVertex),
                           (void*)offsetof(StaticVertex, TexCoords));
}

void TerrainSystem::GenerateLODIndices(TerrainData& data, int chunkSize)
{
    auto& sl = ServiceLocator::Instance();
    auto* gc_ptr = sl.Resolve<IGraphicsContext>();
    if (!gc_ptr)
        return;
    auto& bm = gc_ptr->GetBufferManager();

    for (int lod = 0; lod < 4; ++lod)
    {
        std::vector<unsigned int> indices;
        int step = 1 << lod;

        for (int z = 0; z < chunkSize - 1; z += step)
        {
            for (int x = 0; x < chunkSize - 1; x += step)
            {
                int nextX = x + step;
                int nextZ = z + step;

                if (nextX >= chunkSize)
                    nextX = chunkSize - 1;
                if (nextZ >= chunkSize)
                    nextZ = chunkSize - 1;

                indices.push_back(z * chunkSize + x);
                indices.push_back(nextZ * chunkSize + x);
                indices.push_back(z * chunkSize + nextX);

                indices.push_back(z * chunkSize + nextX);
                indices.push_back(nextZ * chunkSize + x);
                indices.push_back(nextZ * chunkSize + nextX);
            }
        }

        unsigned int ebo = bm.CreateBuffer();
        bm.BindBuffer(BufferType::ElementArrayBuffer, ebo);
        bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(),
                      BufferUsage::StaticDraw);

        data.lodEBOs.push_back(ebo);
        data.lodIndexCounts.push_back((int)indices.size());
    }
}

void TerrainSystem::CleanupTerrainData(TerrainData& data)
{
    auto& sl = ServiceLocator::Instance();
    auto* gc_ptr = sl.Resolve<IGraphicsContext>();
    if (!gc_ptr)
        return;
    auto& bm = gc_ptr->GetBufferManager();

    for (auto& chunk : data.chunks)
    {
        bm.DeleteVertexArray(chunk.VAO);
        bm.DeleteBuffer(chunk.VBO);
    }
    for (auto ebo : data.lodEBOs)
    {
        bm.DeleteBuffer(ebo);
    }
}

std::vector<entt::id_type> TerrainSystem::GetReadComponents() const
{
    return {entt::type_id<TerrainComponent>().hash(), entt::type_id<PositionComponent>().hash()};
}

std::vector<entt::id_type> TerrainSystem::GetWriteComponents() const
{
    return {entt::type_id<TerrainComponent>().hash()};
}

void TerrainSystem::OnTerrainDestroyed(entt::registry& registry, entt::entity entity)
{
    auto* terrain_ptr = registry.try_get<TerrainComponent>(entity);
    if (terrain_ptr)
    {
        if (terrain_ptr->physicsBody)
        {
            auto physics_ptr = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
            if (physics_ptr)
            {
                physics_ptr->RemoveRigidBody(terrain_ptr->physicsBody.get());
            }
            terrain_ptr->physicsBody.reset();
            terrain_ptr->collisionShape.reset();
        }
    }

    auto it = m_TerrainCache.find(entity);
    if (it != m_TerrainCache.end())
    {
        CleanupTerrainData(*it->second);
        m_TerrainCache.erase(it);
    }
}
