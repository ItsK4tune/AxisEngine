#include <ecs/logic/decal_system.h>
#include <ecs/logic/system_factory.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <resource/logic/resource_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_geometry_service.h>
#include <render/unit/gbuffer.h>
#include <core/logic/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ecs/logic/entity_manager.h>
#include <resource/unit/shader.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>
#include <platform/logic/io_handler.h>
#include <render/unit/render_command.h>
#include <render/logic/render_core.h>

REGISTER_SYSTEM(DecalSystem)

void DecalSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<DecalSystem>(this);
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_RenderService = sl.Resolve<IRenderService>();
    m_GeoService = sl.Resolve<IGeometryService>();
    
    auto& resources = sl.Require<ResourceManager>();
    
    m_DecalShader = resources.GetShader("decal");

    m_ForwardShader = resources.GetShader("decal_forward");
}

void DecalSystem::OnDecalConstruct(entt::registry& registry, entt::entity entity)
{
    auto& decal = registry.get<DecalComponent>(entity);
    if (decal.renderOrder == 0) {
        decal.renderOrder = m_NextOrder++;
    }
}

void DecalSystem::Update(Scene &scene, float dt)
{
    static bool logUpdate = false;
    if (!logUpdate) {
        LOGGER_INFO("DecalSystem") << "DecalSystem::Update called for the first time.";
        logUpdate = true;
    }
    m_IsRenderedThisFrame = false;
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
    
    if (!toRemove.empty()) {
        for (auto entity : toRemove) {
            scene.registry.destroy(entity);
        }
    }
}

void DecalSystem::RenderAlphaPass(Scene &scene, int width, int height, float alpha)
{
    if (!m_Enabled) return;
    m_ScreenWidth = width;
    m_ScreenHeight = height;
    Render(scene);
}

void DecalSystem::Render(Scene &scene)
{
    static bool logEntry = false;
    if (!logEntry) {
        LOGGER_INFO("DecalSystem") << "DecalSystem::Render called for the first time.";
        logEntry = true;
    }
    if (!m_Enabled) return;
    
    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService) m_GeoService = sl.Resolve<IGeometryService>();
    if (!m_RenderService) m_RenderService = sl.Resolve<IRenderService>();
    if (!m_GraphicsContext) m_GraphicsContext = sl.Resolve<IGraphicsContext>();

    auto* geoSys = m_GeoService;
    if (!geoSys) {
        static bool logGeo = false;
        if (!logGeo) { LOGGER_ERROR("DecalSystem") << "GeoService not found!"; logGeo = true; }
        return;
    }

    auto camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) {
        static bool logCam = false;
        if (!logCam) { LOGGER_WARN("DecalSystem") << "No active camera found!"; logCam = true; }
        return;
    }
    auto &cam = scene.registry.get<CameraComponent>(camEntity);

    auto& gc = *m_GraphicsContext;
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();
    auto& io = sl.Require<IOHandler>();
    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();
    
    // Safety check for viewport
    if (width <= 0 || height <= 0) return;
    m_ScreenWidth = width;
    m_ScreenHeight = height;

    bool isDeferred = geoSys->IsDeferredRenderingEnabled();
    Shader* shader = isDeferred ? m_DecalShader.get() : m_ForwardShader.get();
    if (!shader) return;

    shader->use();

    if (isDeferred) {
        UpdateTagMap(scene);
        geoSys->BeginDecalPass();
        
        // Use G-Buffer resolution for viewport and shaders
        int gbWidth = (int)geoSys->GetGBufferWidth();
        int gbHeight = (int)geoSys->GetGBufferHeight();
        sm.SetViewport(0, 0, gbWidth, gbHeight);

        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferDepth());
        shader->setInt("gDepth", 0);

        tm.ActiveTexture(TextureUnit::Texture1);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferID());
        shader->setInt("gID", 1);

        tm.ActiveTexture(TextureUnit::Texture2);
        tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
        shader->setInt("tagMap", 2);

        tm.ActiveTexture(TextureUnit::Texture4);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferPosition());
        shader->setInt("gPosition", 4);

        tm.ActiveTexture(TextureUnit::Texture5);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferNormal());
        shader->setInt("gNormalTex", 5);
        
        sm.SetCullFace(CullMode::Front);
        sm.Enable(ServerCapability::CullFace);
        sm.SetDepthFunc(CompareFunc::Always);
        sm.Enable(ServerCapability::Blend); 
        sm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        sm.Disable(ServerCapability::DepthTest);
        sm.SetDepthMask(false); 
        sm.SetDepthMask(false);
        sm.Enable(ServerCapability::CullFace); 
    } else {
        auto* rs = m_RenderService;
        if (rs && geoSys->IsDeferredRenderingEnabled()) return;
        
        uint32_t fbo = rs ? rs->GetMainFBO() : 0;
        gc.GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, fbo);

        sm.Enable(ServerCapability::Blend);
        sm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        sm.Disable(ServerCapability::CullFace);
        sm.SetDepthFunc(CompareFunc::Lequal); // Changed from Less to Lequal
        sm.SetDepthMask(false);
        sm.SetViewport(0, 0, width, height);
        sm.SetColorMask(true, true, true, true);
    }

    shader->setUInt("allowedTagsMask", 0xFFFFFFFF);
    shader->setVec4("tintColor", glm::vec4(1.0f));
    shader->setVec4("u_TintColor", glm::vec4(1.0f));
    shader->setVec4("u_Color", glm::vec4(1.0f));
    
    shader->setMat4("view", cam.viewMatrix);
    shader->setMat4("projection", cam.projectionMatrix);
    shader->setMat4("u_View", cam.viewMatrix);
    shader->setMat4("u_Projection", cam.projectionMatrix);
    
    auto& core = sl.Require<RenderCore>();
    if (isDeferred) {
        sm.Enable(ServerCapability::CullFace);
        bm.BindVertexArray(core.GetCubeVAO());
        bm.BindBuffer(BufferType::ElementArrayBuffer, core.GetCubeEBO());
    } else {
        bm.BindVertexArray(core.GetQuadVAO());
        bm.BindBuffer(BufferType::ElementArrayBuffer, core.GetQuadEBO());
    }
    
    auto view = scene.registry.view<DecalComponent>();

    std::vector<entt::entity> sortedEntities;
    for (auto entity : view) sortedEntities.push_back(entity);

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<DecalComponent>(a).renderOrder < view.get<DecalComponent>(b).renderOrder;
    });

    auto& res = ServiceLocator::Instance().Require<ResourceManager>();

    for (auto entity : sortedEntities) {
        auto &decal = scene.registry.get<DecalComponent>(entity);
        auto *posComp = scene.registry.try_get<PositionComponent>(entity);
        if (!posComp) continue;
        auto &pos = *posComp;
        
        Shader* activeShader = shader;
        if (!decal.customShader.empty()) {
            if (auto custom = res.GetShader(decal.customShader)) {
                activeShader = custom.get();
            }
        }
        
        if (!activeShader) continue;
        activeShader->use();
        
        // Re-set common uniforms for custom shader
        activeShader->setMat4("view", cam.viewMatrix);
        activeShader->setMat4("projection", cam.projectionMatrix);
        activeShader->setMat4("u_View", cam.viewMatrix);
        activeShader->setMat4("u_Projection", cam.projectionMatrix);
        activeShader->setUInt("allowedTagsMask", 0xFFFFFFFF);
        activeShader->setVec4("u_TintColor", glm::vec4(1.0f));

        auto *rotComp = scene.registry.try_get<RotationComponent>(entity);
        auto *scaleComp = scene.registry.try_get<ScaleComponent>(entity);
        
        glm::quat rotation = rotComp ? rotComp->value : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = scaleComp ? scaleComp->value : glm::vec3(1.0f);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos.value) *
                          glm::mat4_cast(rotation) *
                          glm::scale(glm::mat4(1.0f), scale);
        
        if (!isDeferred) model = glm::scale(model, glm::vec3(0.5f));
        
        activeShader->setMat4("model", model);
        activeShader->setMat4("u_Model", model);
        activeShader->setMat4("invModel", glm::inverse(model));
        float finalOpacity = decal.opacity;
        activeShader->setFloat("opacity", finalOpacity);
        activeShader->setFloat("u_Opacity", finalOpacity);
        activeShader->setFloat("u_Alpha", finalOpacity);
        
        if (isDeferred) {
            tm.ActiveTexture(TextureUnit::Texture3);
            tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
            activeShader->setInt("decalAlbedo", 3);
            dc.DrawElements(Primitive::Triangles, 36, DataType::UnsignedInt, 0);
        } else {
            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
            activeShader->setInt("decalAlbedo", 0);
            activeShader->setInt("u_Texture", 0);
            activeShader->setInt("u_Texture0", 0);
            dc.DrawElements(Primitive::Triangles, 6, DataType::UnsignedInt, 0);
        }
    }
    
    if (isDeferred) {
        geoSys->EndDecalPass(m_RenderService ? m_RenderService->GetMainFBO() : 0);
    }
    
    sm.SetColorMask(true, true, true, true);
    sm.SetDepthMask(true);
    sm.SetDepthFunc(CompareFunc::Less);
    sm.Disable(ServerCapability::Blend);
    sm.SetCullFace(CullMode::Back);
}

uint32_t DecalSystem::LoadDecalTexture(const std::string &path)
{
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    auto tex = resources.GetTexture(path);
    if (!tex) {
        resources.LoadTexture(path, path, false);
        tex = resources.GetTexture(path);
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


uint32_t DecalSystem::GetTagBit(const std::string &tag)
{
    if (tag.empty()) return 1; // Bit 0 is for default/untagged
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
    auto& tm = m_GraphicsContext->GetTextureManager();
    if (m_TagMapTexture == 0) m_TagMapTexture = tm.GenTexture();

    uint32_t maxEnt = 10000;
    if (m_TagBuffer.size() < maxEnt) m_TagBuffer.resize(maxEnt, 0);
    else std::fill(m_TagBuffer.begin(), m_TagBuffer.end(), 0);

    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view) {
        uint32_t id = static_cast<uint32_t>(entity) & 0xFFFF;
        if (id < maxEnt) {
            auto& info = view.get<InfoComponent>(entity);
            m_TagBuffer[id] = GetTagBit(info.tag);
        }
    }

    tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
    tm.TexImage1D(TextureType::Texture1D, 0, InternalFormat::R32UI, maxEnt, 0, TextureFormat::Red_Integer, DataType::UnsignedInt, m_TagBuffer.data());
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
}
