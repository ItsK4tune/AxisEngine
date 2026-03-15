#include <ecs/logic/decal_system.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <resource/manager/resource_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/logic/render_system.h>
#include <core/manager/system_manager.h>
#include <core/logic/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ecs/manager/entity_manager.h>
#include <platform/logic/io_handler.h>
#include <render/logic/shader.h>

void DecalSystem::Initialize(EngineContext ctx)
{
    m_Ctx = ctx;
    
    m_DecalShader = m_Ctx.resources->GetShader("decal");
    if (!m_DecalShader)
    {
        m_Ctx.resources->LoadShader("decal", "includes/engine/asset/shaders/decal.vs", 
                                   "includes/engine/asset/shaders/decal.fs");
        m_DecalShader = m_Ctx.resources->GetShader("decal");
    }

    m_ForwardShader = m_Ctx.resources->GetShader("decal_forward");
    if (!m_ForwardShader)
    {
        m_Ctx.resources->LoadShader("decal_forward", "includes/engine/asset/shaders/decal_forward.vs", 
                                   "includes/engine/asset/shaders/decal_forward.fs");
        m_ForwardShader = m_Ctx.resources->GetShader("decal_forward");
    }

    if (m_Ctx.scene) {
        m_Ctx.scene->registry.on_construct<DecalComponent>().connect<&DecalSystem::OnDecalConstruct>(this);
    }
}

void DecalSystem::OnDecalConstruct(entt::registry& registry, entt::entity entity)
{
    auto& decal = registry.get<DecalComponent>(entity);
    if (decal.renderOrder == 0) {
        decal.renderOrder = m_NextOrder++;
    }
    LOGGER_INFO("DecalSystem") << "Decal created: Entity=" << (uint32_t)entity << ", Lifetime=" << decal.lifetime << ", Order=" << decal.renderOrder;
}

void DecalSystem::Initialize(IGraphicsContext &context, IShaderLibrary &shaderLib)
{
    InitCubeMesh();
    LOGGER_INFO("DecalSystem") << "DecalSystem GPU resources initialized.";
}

void DecalSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled) return;
    
    auto view = scene.registry.view<DecalComponent>();
    std::vector<entt::entity> toRemove;
    
    for (auto entity : view) {
        auto &decal = view.get<DecalComponent>(entity);
        if (decal.lifetime > 0.0f) {
            decal.lifetime -= dt;
            if (decal.lifetime <= 0.0f) {
                toRemove.push_back(entity);
            }
        }
    }
    
    // Sort toRemove by renderOrder to ensure stable disappearance even if multiple decals hit O in the same frame
    std::sort(toRemove.begin(), toRemove.end(), [&](entt::entity a, entt::entity b) {
        return scene.registry.get<DecalComponent>(a).renderOrder < scene.registry.get<DecalComponent>(b).renderOrder;
    });

    for (auto entity : toRemove) {
        auto &decal = scene.registry.get<DecalComponent>(entity);
        LOGGER_INFO("DecalSystem") << "Decal destroyed (lifetime expired): Entity=" << (uint32_t)entity << ", Lifetime=" << decal.lifetime << ", Order=" << decal.renderOrder;
        scene.registry.destroy(entity);
    }
}

void DecalSystem::Render(Scene &scene)
{
    if (!m_Enabled) return;
    
    auto rs = m_Ctx.systems->GetSystem<RenderSystem>();
    if (!rs) return;

    auto camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;
    auto &cam = scene.registry.get<CameraComponent>(camEntity);

    auto& gc = m_Ctx.io->GetGraphicsContext();
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();

    bool isDeferred = rs->IsDeferredRenderingEnabled();
    Shader* shader = isDeferred ? m_DecalShader.get() : m_ForwardShader.get();
    if (!shader) return;

    if (isDeferred) rs->BindForDecals();
    
    shader->use();
    
    if (isDeferred) {
        // Screen Space Uniforms
        shader->setMat4("invProj", glm::inverse(cam.projectionMatrix));
        shader->setMat4("invView", glm::inverse(cam.viewMatrix));
        shader->setVec2("screenSize", glm::vec2((float)rs->GetGBufferWidth(), (float)rs->GetGBufferHeight()));

        UpdateTagMap(scene);

        // Bind G-Buffer
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, rs->GetGBufferDepth());
        shader->setInt("gDepth", 0);

        tm.ActiveTexture(TextureUnit::Texture1);
        tm.BindTexture(TextureType::Texture2D, rs->GetGBufferID());
        shader->setInt("gID", 1);

        tm.ActiveTexture(TextureUnit::Texture2);
        tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
        shader->setInt("tagMap", 2);

        tm.ActiveTexture(TextureUnit::Texture4);
        tm.BindTexture(TextureType::Texture2D, rs->GetGBufferPosition());
        shader->setInt("gPosition", 4);
        
        sm.SetCullFace(CullMode::Front); // Back faces for volume
        sm.SetDepthFunc(CompareFunc::Always);
    } else {
        // Forward Mesh Decal Uniforms
        shader->setVec4("tintColor", glm::vec4(1.0f));
        sm.SetCullFace(CullMode::Back);
        sm.SetDepthFunc(CompareFunc::Less);
    }

    shader->setMat4("view", cam.viewMatrix);
    shader->setMat4("projection", cam.projectionMatrix);
    
    sm.Enable(ServerCapability::DepthTest);
    sm.SetDepthMask(false);
    sm.Enable(ServerCapability::CullFace);
    sm.Enable(ServerCapability::Blend);
    sm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    bm.BindVertexArray(m_CubeVAO);
    bm.BindBuffer(BufferType::ElementArrayBuffer, m_CubeEBO);
    
    auto view = scene.registry.view<DecalComponent, PositionComponent, RotationComponent, ScaleComponent>();
    
    // Sort entities by renderOrder to ensure correct overlapping
    std::vector<entt::entity> sortedEntities;
    sortedEntities.reserve(view.size_hint());
    for (auto entity : view) sortedEntities.push_back(entity);

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<DecalComponent>(a).renderOrder < view.get<DecalComponent>(b).renderOrder;
    });

    for (auto entity : sortedEntities) {
        auto &decal = view.get<DecalComponent>(entity);
        auto &pos = view.get<PositionComponent>(entity);
        auto &rot = view.get<RotationComponent>(entity);
        auto &scale = view.get<ScaleComponent>(entity);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value) *
                          glm::mat4_cast(rot.value) *
                          glm::scale(glm::mat4(1.0f), scale.value);
        
        shader->setMat4("model", model);
        shader->setMat4("invModel", glm::inverse(model));
        shader->setFloat("opacity", decal.opacity);
        
        tm.ActiveTexture(TextureUnit::Texture3);
        tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
        shader->setInt("decalAlbedo", 3);

        if (isDeferred) {
            uint32_t mask = GetBitmask(decal.targetTags);
            shader->setUInt("allowedTagsMask", mask);
            dc.DrawElements(Primitive::Triangles, 36, DataType::UnsignedInt, nullptr);
        } else {
            // In forward mode, we only render the "front" face of the cube as a quad
            // Front face indices in the cube are 0, 1, 2, 2, 3, 0 (first 6 indices)
            dc.DrawElements(Primitive::Triangles, 6, DataType::UnsignedInt, nullptr);
        }
    }
    
    if (isDeferred) rs->UnbindForDecals();
    
    sm.SetDepthMask(true);
    sm.SetDepthFunc(CompareFunc::Less);
    sm.Disable(ServerCapability::Blend);
    sm.SetCullFace(CullMode::Back);
}

uint32_t DecalSystem::LoadDecalTexture(const std::string &path)
{
    auto tex = m_Ctx.resources->GetTexture(path);
    if (!tex) {
        m_Ctx.resources->LoadTexture(path, path, false);
        tex = m_Ctx.resources->GetTexture(path);
    }
    return tex ? tex->id : 0;
}

void DecalSystem::FlushDecals(Scene &scene)
{
    auto view = scene.registry.view<DecalComponent>();
    scene.registry.destroy(view.begin(), view.end());
}

std::vector<entt::id_type> DecalSystem::GetReadComponents() const
{
    return {
        entt::type_id<DecalComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash()
    };
}

std::vector<entt::id_type> DecalSystem::GetWriteComponents() const
{
    return {
        entt::type_id<DecalComponent>().hash()
    };
}

void DecalSystem::InitCubeMesh()
{
    auto& gc = m_Ctx.io->GetGraphicsContext();
    auto& bm = gc.GetBufferManager();
    
    float vertices[] = {
        // Front
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // Back
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0,
        1, 5, 6,  6, 2, 1,
        7, 6, 5,  5, 4, 7,
        4, 0, 3,  3, 7, 4,
        4, 5, 1,  1, 0, 4,
        3, 2, 6,  6, 7, 3
    };
    
    m_CubeVAO = bm.CreateVertexArray();
    m_CubeVBO = bm.CreateBuffer();
    m_CubeEBO = bm.CreateBuffer();
    
    bm.BindVertexArray(m_CubeVAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_CubeVBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(vertices), vertices, BufferUsage::StaticDraw);
    
    bm.BindBuffer(BufferType::ElementArrayBuffer, m_CubeEBO);
    bm.BufferData(BufferType::ElementArrayBuffer, sizeof(indices), indices, BufferUsage::StaticDraw);
    
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 3 * sizeof(float), (void*)0);
}

uint32_t DecalSystem::GetTagBit(const std::string &tag)
{
    if (tag.empty()) return 0;
    auto it = m_TagBitMap.find(tag);
    if (it != m_TagBitMap.end()) return it->second;

    uint32_t bit = 1 << m_TagBitMap.size();
    m_TagBitMap[tag] = bit;
    return bit;
}

uint32_t DecalSystem::GetBitmask(const std::vector<std::string> &tags)
{
    uint32_t mask = 0;
    for (const auto& tag : tags) mask |= GetTagBit(tag);
    return mask;
}

void DecalSystem::UpdateTagMap(Scene &scene)
{
    auto& tm = m_Ctx.io->GetGraphicsContext().GetTextureManager();
    
    if (m_TagMapTexture == 0) {
        m_TagMapTexture = tm.GenTexture();
    }

    uint32_t maxEnt = 10000; // Increased capacity for demo
    if (m_TagBuffer.size() < maxEnt) {
        m_TagBuffer.resize(maxEnt, 0);
    } else {
        std::fill(m_TagBuffer.begin(), m_TagBuffer.end(), 0);
    }

    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view) {
        uint32_t id = static_cast<uint32_t>(entity) & 0xFFFF; // Simple index
        if (id < maxEnt) {
            auto& info = view.get<InfoComponent>(entity);
            m_TagBuffer[id] = GetTagBit(info.tag);
        }
    }

    tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
    tm.TexImage1D(TextureType::Texture1D, 0, InternalFormat::R32UI, maxEnt, 0, TextureFormat::Red_Integer, DataType::UnsignedInt, m_TagBuffer.data());
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
}
