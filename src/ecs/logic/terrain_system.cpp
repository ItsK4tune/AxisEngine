#include <ecs/logic/terrain_system.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/logic/frustum_culler.h>
#include <resource/logic/resource_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <resource/unit/shader.h>
#include <physics/interface/i_physics_world.h>
#include <ecs/unit/terrain_component.h>
#include <core/logic/service_locator.h>

void TerrainSystem::Initialize() {
    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All)) {

        }
    });
}

void TerrainSystem::Shutdown() {
    for (auto& pair : m_TerrainCache) {
        CleanupTerrainData(*pair.second);
    }
    m_TerrainCache.clear();
}

void TerrainSystem::Update(Scene &scene, float dt) {
    if (!m_Enabled) return;

    auto view = scene.registry.view<TerrainComponent>();
    for (auto entity : view) {
        auto &terrain = view.get<TerrainComponent>(entity);
        if (terrain.needsRebuild) {
            LOGGER_INFO("TerrainSystem") << "Rebuilding terrain for entity " << (uint32_t)entity;
            BuildTerrain(entity, terrain);
        }
    }
}

void TerrainSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
    if (!m_Enabled) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    auto& sl = ServiceLocator::Instance();
    auto& gc = sl.Require<IGraphicsContext>();
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();
    auto& resources = sl.Require<ResourceManager>();

    FrustumCuller culler;
    culler.BuildFrustum(cam.projectionMatrix * cam.viewMatrix);

    auto* geoSys = sl.Resolve<IGeometryService>();

    auto view = scene.registry.view<TerrainComponent, PositionComponent>();
    for (auto entity : view) {
        auto &terrain = view.get<TerrainComponent>(entity);
        auto &pos = view.get<PositionComponent>(entity);
        
        auto it = m_TerrainCache.find(entity);
        if (it == m_TerrainCache.end()) {
            continue;
        }

        TerrainData& data = *it->second;
        Shader* actShader = data.terrainShader.get();
        if (!terrain.customShader.empty()) {
            if (auto custom = resources.GetShader(terrain.customShader)) {
                actShader = custom.get();
            }
        }
        
        if (geoSys && geoSys->IsDeferredRenderingEnabled()) {
             auto gbufShader = resources.GetShader("terrain_gbuffer");
             if (!terrain.customShader.empty()) {
                 if (auto custom = resources.GetShader(terrain.customShader + "_gbuffer")) {
                     gbufShader = custom;
                 }
             }

             if (gbufShader) actShader = gbufShader.get();
             geoSys->BindGBufferForWriting();
        }

        if (!actShader) continue;

        actShader->use();
        actShader->setMat4("view", cam.viewMatrix);
        actShader->setMat4("projection", cam.projectionMatrix);
        actShader->setVec3("camPos", camPos);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value);
        actShader->setMat4("model", model);
        actShader->setUInt("entityID", static_cast<unsigned int>(entity));
        
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

        for (size_t i = 0; i < terrain.diffuseLayers.size() && i < 4; ++i) {
            tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture28) + i));
            tm.BindTexture(TextureType::Texture2D, terrain.diffuseLayers[i]);
            std::string name = "textureLayer" + std::to_string(i);
            actShader->setInt(name.c_str(), 28 + (int)i);
        }

        for (auto& chunk : data.chunks) {
            if (!culler.IsVisible(chunk.minBound + pos.value, chunk.maxBound + pos.value)) {
                continue;
            }

            float dist = glm::distance(camPos, (chunk.minBound + chunk.maxBound) * 0.5f + pos.value);
            int lod = 0;
            if (dist > 300.0f) lod = 3;
            else if (dist > 150.0f) lod = 2;
            else if (dist > 75.0f) lod = 1;

            if (lod >= (int)data.lodEBOs.size()) continue;

            bm.BindVertexArray(chunk.VAO);
            bm.BindBuffer(BufferType::ElementArrayBuffer, data.lodEBOs[lod]);
            dc.DrawElements(Primitive::Triangles, data.lodIndexCounts[lod], DataType::UnsignedInt, nullptr);
            auto* rs = sl.Resolve<IRenderService>();
            if (rs) rs->AddRenderedCount(1);
        }

        if (geoSys && geoSys->IsDeferredRenderingEnabled()) {
            geoSys->UnbindGBuffer();
        }
    }
}

void TerrainSystem::BuildTerrain(entt::entity entity, TerrainComponent& terrain) {
    auto& sl = ServiceLocator::Instance();
    auto& resources = sl.Require<ResourceManager>();

    auto data = std::make_unique<TerrainData>();
    
    data->terrainShader = resources.GetShader("terrain");

    if (!data->terrainShader) {
        LOGGER_ERROR("TerrainSystem") << "Cannot build terrain: shader failed to load.";
        return;
    }

    GenerateLODIndices(*data, terrain.chunkSize);

    int numChunksX = (terrain.resolution - 1) / (terrain.chunkSize - 1);
    int numChunksZ = (terrain.resolution - 1) / (terrain.chunkSize - 1);

    for (int z = 0; z < numChunksZ; ++z) {
        for (int x = 0; x < numChunksX; ++x) {
            TerrainChunk chunk;
            chunk.gridPos = glm::ivec2(x, z);
            CreateChunkMesh(chunk, terrain, x * (terrain.chunkSize - 1), z * (terrain.chunkSize - 1));
            data->chunks.push_back(chunk);
        }
    }

    auto cacheIt = m_TerrainCache.find(entity);
    if (cacheIt != m_TerrainCache.end() && cacheIt->second) {
        CleanupTerrainData(*cacheIt->second);
    }

    m_TerrainCache[entity] = std::move(data);
    
    bool needsRetry = false;
    LOGGER_INFO("TerrainSystem") << "Checking physics generation for entity " << (uint32_t)entity << ": generate=" << terrain.generatePhysics << ", mapName=" << terrain.heightMapName;
    
    auto physics_ptr = sl.Resolve<IPhysicsWorld>();
    if (terrain.generatePhysics && physics_ptr && !terrain.heightMapName.empty()) {
        auto tex = resources.GetTexture(terrain.heightMapName);
        if (tex) {
            if (tex->pixelData) {
                std::vector<float> heights;
                heights.reserve(tex->width * tex->height);
                
                for (int i = 0; i < tex->width * tex->height; ++i) {
                    float hValue = (float)tex->pixelData[i * tex->nrComponents] / 255.0f;
                    heights.push_back(hValue * terrain.maxHeight);
                }
                
                if (terrain.physicsBody) {
                    physics_ptr->RemoveRigidBody(terrain.physicsBody.get());
                    terrain.physicsBody.reset();
                }

                if (terrain.collisionShape) {
                    terrain.collisionShape.reset();
                }

                terrain.collisionShape = physics_ptr->CreateHeightfieldShape(
                    heights, tex->width, tex->height, 0.0f, terrain.maxHeight);
                
                if (terrain.collisionShape) {
                    float scaleX = terrain.terrainSize.x / (float)(tex->width > 1 ? tex->width - 1 : 1);
                    float scaleZ = terrain.terrainSize.z / (float)(tex->height > 1 ? tex->height - 1 : 1);
                    terrain.collisionShape->SetLocalScaling(glm::vec3(scaleX, 1.0f, scaleZ));

                    glm::vec3 basePos(0.0f);

                    
                    glm::vec3 centerPos = basePos + glm::vec3(terrain.terrainSize.x * 0.5f, terrain.maxHeight * 0.5f, terrain.terrainSize.z * 0.5f);

                    terrain.physicsBody = physics_ptr->CreateRigidBody(0.0f, centerPos, glm::quat(1,0,0,0), terrain.collisionShape);
                    
                    if (terrain.physicsBody) {
                        terrain.physicsBody->SetUserPointer((void*)((uintptr_t)entity + 1));
                        physics_ptr->AddRigidBody(terrain.physicsBody.get());
                    }
                }
            } else {
                needsRetry = true;
            }
        } else {
            needsRetry = true;
        }
    }

    if (!needsRetry) {
        terrain.needsRebuild = false;
    }
}

void TerrainSystem::CreateChunkMesh(TerrainChunk& chunk, const TerrainComponent& terrain, int xOffset, int zOffset) {
    auto& gc = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = gc.GetBufferManager();

    std::vector<Vertex> vertices;
    int size = terrain.chunkSize;
    float stepX = terrain.terrainSize.x / (float)(terrain.resolution - 1);
    float stepZ = terrain.terrainSize.z / (float)(terrain.resolution - 1);

    chunk.minBound = glm::vec3(xOffset * stepX, 0.0f, zOffset * stepZ);
    chunk.maxBound = glm::vec3((xOffset + size - 1) * stepX, terrain.maxHeight, (zOffset + size - 1) * stepZ);

    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            Vertex v;
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                v.m_BoneIDs[i] = 0;
                v.m_Weights[i] = 0.0f;
            }
            v.Tangent = glm::vec3(1, 0, 0);
            v.Bitangent = glm::vec3(0, 0, 1);
            float worldX = (xOffset + x) * stepX;
            float worldZ = (zOffset + z) * stepZ;
            
            v.Position = glm::vec3(worldX, 0.0f, worldZ);
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.TexCoords = glm::vec2((float)(xOffset + x) / (float)(terrain.resolution - 1), 
                                   (float)(zOffset + z) / (float)(terrain.resolution - 1));
            
            vertices.push_back(v);
        }
    }

    chunk.VAO = bm.CreateVertexArray();
    chunk.VBO = bm.CreateBuffer();

    bm.BindVertexArray(chunk.VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, chunk.VBO);
    bm.BufferData(BufferType::ArrayBuffer, vertices.size() * sizeof(Vertex), vertices.data(), BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
}

void TerrainSystem::GenerateLODIndices(TerrainData& data, int chunkSize) {
    auto& sl = ServiceLocator::Instance();
    auto& gc = sl.Require<IGraphicsContext>();
    auto& bm = gc.GetBufferManager();
    
    for (int lod = 0; lod < 4; ++lod) {
        std::vector<unsigned int> indices;
        int step = 1 << lod;
        
        for (int z = 0; z < chunkSize - 1; z += step) {
            for (int x = 0; x < chunkSize - 1; x += step) {
                int nextX = x + step;
                int nextZ = z + step;
                
                if (nextX >= chunkSize) nextX = chunkSize - 1;
                if (nextZ >= chunkSize) nextZ = chunkSize - 1;

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
        bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(), BufferUsage::StaticDraw);
        
        data.lodEBOs.push_back(ebo);
        data.lodIndexCounts.push_back((int)indices.size());
    }
}

void TerrainSystem::CleanupTerrainData(TerrainData& data) {
    auto& sl = ServiceLocator::Instance();
    auto& gc = sl.Require<IGraphicsContext>();
    auto& bm = gc.GetBufferManager();

    for (auto& chunk : data.chunks) {
        bm.DeleteVertexArray(chunk.VAO);
        bm.DeleteBuffer(chunk.VBO);
    }
    for (auto ebo : data.lodEBOs) {
        bm.DeleteBuffer(ebo);
    }
}

std::vector<entt::id_type> TerrainSystem::GetReadComponents() const
{
    return {
        entt::type_id<TerrainComponent>().hash(),
        entt::type_id<PositionComponent>().hash()
    };
}

std::vector<entt::id_type> TerrainSystem::GetWriteComponents() const
{
    return {
        entt::type_id<TerrainComponent>().hash()
    };
}
