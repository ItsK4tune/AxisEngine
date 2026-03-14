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
    
    for (auto entity : toRemove) {
        scene.registry.destroy(entity);
    }
}

void DecalSystem::Render(Scene &scene)
{
    if (!m_Enabled || !m_DecalShader) return;
    
    auto rs = m_Ctx.systems->GetSystem<RenderSystem>();
    if (!rs || !rs->IsDeferredRenderingEnabled()) return;
    
    m_DecalShader->use();
    
    auto camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    rs->BindForDecals();
    
    auto &cam = scene.registry.get<CameraComponent>(camEntity);
    
    auto& gc = m_Ctx.io->GetGraphicsContext();
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();
    
    m_DecalShader->use();
    
    // Set Camera Uniforms
    m_DecalShader->setMat4("invProj", glm::inverse(cam.projectionMatrix));
    m_DecalShader->setMat4("invView", glm::inverse(cam.viewMatrix));
    m_DecalShader->setMat4("view", cam.viewMatrix);
    m_DecalShader->setMat4("projection", cam.projectionMatrix);
    
    int width = rs->GetGBufferWidth();
    int height = rs->GetGBufferHeight();
    m_DecalShader->setVec2("screenSize", glm::vec2((float)width, (float)height));
    
    UpdateTagMap(scene);

    // Bind G-Buffer Depth (unit 0)
    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, rs->GetGBufferDepth());
    m_DecalShader->setInt("gDepth", 0);

    // Bind G-Buffer ID (unit 1)
    tm.ActiveTexture(TextureUnit::Texture1);
    tm.BindTexture(TextureType::Texture2D, rs->GetGBufferID());
    m_DecalShader->setInt("gID", 1);

    // Bind Tag Map (unit 2)
    tm.ActiveTexture(TextureUnit::Texture2);
    tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
    m_DecalShader->setInt("tagMap", 2);

    // Bind G-Buffer Position (unit 4)
    tm.ActiveTexture(TextureUnit::Texture4);
    tm.BindTexture(TextureType::Texture2D, rs->GetGBufferPosition());
    m_DecalShader->setInt("gPosition", 4);
    
    m_DecalShader->setVec2("screenSize", glm::vec2((float)rs->GetGBufferWidth(), (float)rs->GetGBufferHeight()));
    
    // Rendering setup
    sm.Enable(ServerCapability::DepthTest);
    sm.SetDepthFunc(CompareFunc::Always);
    sm.SetDepthMask(false); // Don't write to depth
    sm.Enable(ServerCapability::CullFace);
    sm.SetCullFace(CullMode::Front); // Render back faces to ensure volume coverage
    
    sm.Enable(ServerCapability::Blend);
    sm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    bm.BindVertexArray(m_CubeVAO);
    bm.BindBuffer(BufferType::ElementArrayBuffer, m_CubeEBO);
    
    auto view = scene.registry.view<DecalComponent, PositionComponent, RotationComponent, ScaleComponent>();
    for (auto entity : view) {
        auto &decal = view.get<DecalComponent>(entity);
        auto &pos = view.get<PositionComponent>(entity);
        auto &rot = view.get<RotationComponent>(entity);
        auto &scale = view.get<ScaleComponent>(entity);
        
        // Calculate matrices
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value) *
                          glm::mat4_cast(rot.value) *
                          glm::scale(glm::mat4(1.0f), scale.value);
        
        m_DecalShader->setMat4("view", cam.viewMatrix);
        m_DecalShader->setMat4("projection", cam.projectionMatrix);
        m_DecalShader->setMat4("model", model);
        m_DecalShader->setMat4("invModel", glm::inverse(model));
        m_DecalShader->setFloat("opacity", decal.opacity);
        
        tm.ActiveTexture(TextureUnit::Texture3);
        tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
        m_DecalShader->setInt("decalAlbedo", 3);

        uint32_t mask = GetBitmask(decal.targetTags);
        m_DecalShader->setUInt("allowedTagsMask", mask);
        
        dc.DrawElements(Primitive::Triangles, 36, DataType::UnsignedInt, nullptr);
    }
    
    rs->UnbindForDecals();
    
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
