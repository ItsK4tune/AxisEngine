#include <ecs/logic/decal_system.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/shadow_system.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <platform/logic/io_handler.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/render_core.h>
#include <render/unit/gbuffer.h>
#include <render/unit/render_command.h>
#include <render/unit/render_queue.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/shader.h>
#include <scene/logic/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>


void DecalSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<DecalSystem>(this);
    m_GraphicsContext = sl.Resolve<IGraphicsContext>();
    m_RenderService = sl.Resolve<IRenderService>();
    m_GeoService = sl.Resolve<IGeometryService>();

    auto* resources = sl.Resolve<ResourceManager>();
    if (!resources)
    {
        LOGGER_WARN("DecalSystem") << "Skipping full initialization (missing ResourceManager)";
        return;
    }

    m_DecalShader = resources->GetShader("deferred_decal");
    m_ForwardShader = resources->GetShader("forward_decal");

    if (!m_DecalShader || !m_ForwardShader)
    {
        LOGGER_ERROR("DecalSystem") << "FAILED to load decal shaders! "
                                    << "[Deferred: " << (m_DecalShader ? "OK" : "MISSING") << ", "
                                    << "Forward: " << (m_ForwardShader ? "OK" : "MISSING") << "]";
    }
    else
    {
        LOGGER_INFO("DecalSystem") << "Shaders initialized: "
                                   << "[Deferred: " << m_DecalShader->getID() << ", "
                                   << "Forward: " << m_ForwardShader->getID() << "]";
    }
}

void DecalSystem::Shutdown()
{
    if (m_BoundScene)
        m_BoundScene->GetRegistry().on_construct<DecalComponent>().disconnect<&DecalSystem::OnDecalConstruct>(this);
    m_BoundScene = nullptr;
    if (m_GraphicsContext && m_TagMapTexture != 0)
        m_GraphicsContext->GetTextureManager().DeleteTextures(1, &m_TagMapTexture);
    m_TagMapTexture = 0;
    m_TagBuffer.clear();
    m_TagBitMap.clear();
    m_GraphicsContext = nullptr;
    m_GeoService = nullptr;
    m_RenderService = nullptr;
}

void DecalSystem::OnDecalConstruct(entt::registry& registry, entt::entity entity)
{
    auto& decal = registry.get<DecalComponent>(entity);
    if (decal.renderOrder == 0)
    {
        decal.renderOrder = m_NextOrder++;
    }
}

void DecalSystem::Update(Scene& scene, float dt)
{
    if (m_BoundScene != &scene)
    {
        if (m_BoundScene)
            m_BoundScene->GetRegistry().on_construct<DecalComponent>().disconnect<&DecalSystem::OnDecalConstruct>(this);
        m_BoundScene = &scene;
        auto& registry = scene.GetRegistry();
        registry.on_construct<DecalComponent>().connect<&DecalSystem::OnDecalConstruct>(this);
        auto existing = registry.view<DecalComponent>();
        for (auto entity : existing) OnDecalConstruct(registry, entity);
    }
    if (!m_Enabled)
        return;

    auto view = scene.View<DecalComponent>();
    std::vector<entt::entity> toRemove;

    for (auto entity : view)
    {
        auto& decal = view.get<DecalComponent>(entity);
        if (decal.lifetime > 0.0f)
        {
            decal.lifetime -= dt;
            if (decal.lifetime <= 0.0f)
            {
                toRemove.push_back(entity);
            }
        }
    }

    if (!toRemove.empty())
    {
        for (auto entity : toRemove)
        {
            scene.Destroy(entity);
        }
    }
}

void DecalSystem::RenderAlphaPass(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService)
        m_GeoService = sl.Resolve<IGeometryService>();

    if (m_GeoService && m_GeoService->IsDeferredRenderingEnabled())
        return;

    RenderDecals(scene, false);
}

void DecalSystem::Render(Scene& scene)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService)
        m_GeoService = sl.Resolve<IGeometryService>();

    if (!m_GeoService || !m_GeoService->IsDeferredRenderingEnabled())
        return;

    RenderDecals(scene, true);
}

void DecalSystem::RenderDecals(Scene& scene, bool isDeferred)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    if (!m_GeoService)
        m_GeoService = sl.Resolve<IGeometryService>();
    if (!m_RenderService)
        m_RenderService = sl.Resolve<IRenderService>();
    if (!m_GraphicsContext)
        m_GraphicsContext = sl.Resolve<IGraphicsContext>();

    auto* geoSys = m_GeoService;
    if (isDeferred && !geoSys)
        return;

    auto camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;
    auto& cam = scene.GetComponent<CameraComponent>(camEntity);

    if (!m_GraphicsContext)
        return;
    auto& gc = *m_GraphicsContext;
    auto& dc = gc.GetDrawContext();
    auto& bm = gc.GetBufferManager();
    auto& tm = gc.GetTextureManager();
    auto& sm = gc.GetRenderStateManager();

    auto* io = sl.Resolve<IOHandler>();
    int width = io ? io->GetMonitorManager().GetWidth() : 800;
    int height = io ? io->GetMonitorManager().GetHeight() : 600;

    // Safety check for viewport
    if (width <= 0 || height <= 0)
        return;
    Shader* shader = isDeferred ? m_DecalShader.get() : m_ForwardShader.get();
    if (!shader)
        return;

    shader->use();

    if (isDeferred)
    {
        UpdateTagMap(scene);
        geoSys->BeginDecalPass();

        // Use G-Buffer resolution for viewport and shaders
        int gbWidth = (int)geoSys->GetGBufferWidth();
        int gbHeight = (int)geoSys->GetGBufferHeight();
        sm.SetViewport(0, 0, gbWidth, gbHeight);

        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferDepth());
        shader->setInt("u_GDepth", 0);

        tm.ActiveTexture(TextureUnit::Texture1);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferID());
        shader->setInt("u_GID", 1);

        tm.ActiveTexture(TextureUnit::Texture2);
        tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
        shader->setInt("u_TagMap", 2);

        tm.ActiveTexture(TextureUnit::Texture5);
        tm.BindTexture(TextureType::Texture2D, geoSys->GetGBufferNormal());
        shader->setInt("u_GNormalTex", 5);

        sm.SetCullFace(CullMode::Front);
        sm.Enable(ServerCapability::CullFace);
        sm.SetDepthFunc(CompareFunc::Always);
        sm.Enable(ServerCapability::Blend);
        // Deferred MRT alpha channels carry surface metadata (Fresnel, receive-shadow and
        // reflection-probe flags). Blend only RGB and preserve those destination values.
        sm.SetBlendFuncSeparate(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendFactor::Zero,
                                BlendFactor::One);
        sm.Disable(ServerCapability::DepthTest);
        sm.SetDepthMask(false);
        sm.Enable(ServerCapability::CullFace);
    }
    else
    {
        auto* rs = m_RenderService;
        uint32_t fbo = rs ? rs->GetMainFBO() : 0;
        gc.GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, fbo);

        sm.Enable(ServerCapability::Blend);
        sm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
        sm.Disable(ServerCapability::CullFace);
        sm.SetDepthFunc(CompareFunc::Lequal);  // Changed from Less to Lequal
        sm.SetDepthMask(false);
        sm.SetViewport(0, 0, width, height);
        sm.SetColorMask(true, true, true, true);

        // Bind Shadows for Forward Mode
        auto* shadowSys = sl.Resolve<ShadowSystem>();
        if (shadowSys && shadowSys->GetRenderer().IsShadowsEnabled())
        {
            shadowSys->GetShadow().BindTexture_Dir(0, 10);
            shadowSys->GetShadow().BindTexture_Point(0, 11);
            shadowSys->GetShadow().BindTexture_Spot(0, 12);
            shader->setInt("u_ShadowMapDir", 10);
            shader->setInt("u_ShadowMapPoint", 11);
            shader->setInt("u_ShadowMapSpot", 12);
        }

        auto* lightingService = sl.Resolve<ILightingService>();
        if (lightingService && rs)
        {
            RenderSceneData sceneData;
            sceneData.lightView = &rs->GetRenderQueueObj().GetLights();
            sceneData.cameraPosition = rs->GetCameraPosition();
            sceneData.viewMatrix = rs->GetViewMatrix();
            sceneData.projMatrix = rs->GetProjectionMatrix();
            sceneData.nearPlane = rs->GetNearPlane();
            sceneData.farPlane = rs->GetFarPlane();
            lightingService->UploadLightData(sceneData, shader);
        }
    }

    shader->setVec4("u_TintColor", glm::vec4(1.0f));
    shader->setFloat("u_Roughness", 1.0f);
    shader->setFloat("u_Metallic", 0.0f);
    shader->setFloat("u_Reflectivity", 0.0f);
    shader->setMat4("u_View", cam.viewMatrix);
    shader->setMat4("u_Projection", cam.projectionMatrix);

    auto* core = sl.Resolve<RenderCore>();
    if (core)
    {
        if (isDeferred)
        {
            sm.Enable(ServerCapability::CullFace);
            bm.BindVertexArray(core->GetCubeVAO());
            bm.BindBuffer(BufferType::ElementArrayBuffer, core->GetCubeEBO());
        }
        else
        {
            bm.BindVertexArray(core->GetQuadVAO());
            bm.BindBuffer(BufferType::ElementArrayBuffer, core->GetQuadEBO());
        }
    }
    else
    {
        return;
    }

    auto view = scene.View<DecalComponent>();

    std::vector<entt::entity> sortedEntities;
    for (auto entity : view) sortedEntities.push_back(entity);

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<DecalComponent>(a).renderOrder < view.get<DecalComponent>(b).renderOrder;
    });

    auto* res = ServiceLocator::Instance().Resolve<ResourceManager>();
    if (!res)
        return;

    for (auto entity : sortedEntities)
    {
        auto& decal = scene.GetComponent<DecalComponent>(entity);
        auto* posComp = scene.TryGetComponent<PositionComponent>(entity);
        if (!posComp)
            continue;
        auto& pos = *posComp;

        Shader* activeShader = shader;
        if (!decal.customShader.empty())
        {
            if (auto custom = res->GetShader(decal.customShader))
            {
                activeShader = custom.get();
            }
        }

        if (!activeShader)
            continue;
        activeShader->use();

        // Uniform locations are program-specific, so custom shaders must receive
        // the complete deferred input contract after switching programs.
        activeShader->setMat4("u_View", cam.viewMatrix);
        activeShader->setMat4("u_Projection", cam.projectionMatrix);
        if (isDeferred)
        {
            activeShader->setInt("u_GDepth", 0);
            activeShader->setInt("u_GID", 1);
            activeShader->setInt("u_TagMap", 2);
            activeShader->setInt("u_GNormalTex", 5);
            const uint32_t allowedTags = decal.targetTags.empty() ? 0xFFFFFFFFu : GetBitmask(decal.targetTags);
            activeShader->setUInt("u_AllowedTagsMask", allowedTags);
        }

        auto* rotComp = scene.TryGetComponent<RotationComponent>(entity);
        auto* scaleComp = scene.TryGetComponent<ScaleComponent>(entity);

        glm::quat rotation = rotComp ? rotComp->value : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = scaleComp ? scaleComp->value : glm::vec3(1.0f);

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), pos.value) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);

        if (!isDeferred)
            model = glm::scale(model, glm::vec3(0.5f));

        activeShader->setMat4("u_Model", model);
        if (isDeferred)
            activeShader->setMat4("u_InvModel", glm::inverse(model));
        float finalOpacity = decal.opacity;
        activeShader->setFloat("u_Opacity", finalOpacity);
        activeShader->setVec4("u_TintColor", decal.tintColor);
        activeShader->setFloat("u_Roughness", decal.roughness);
        activeShader->setFloat("u_Metallic", decal.metallic);
        activeShader->setFloat("u_Reflectivity", decal.reflectivity);
        activeShader->setInt("u_LightingMode", decal.lightingMode);

        if (decal.albedoMap == 0 && !decal.albedoTexture.empty())
        {
            if (auto texture = res->GetTextureAuto(decal.albedoTexture))
                decal.albedoMap = texture->id;
        }

        if (isDeferred)
        {
            tm.ActiveTexture(TextureUnit::Texture3);
            tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
            activeShader->setInt("u_DecalAlbedo", 3);
            activeShader->setBool("u_HasDecalTexture", decal.albedoMap != 0);
            dc.DrawElements(Primitive::Triangles, 36, DataType::UnsignedInt, 0);
        }
        else
        {
            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::Texture2D, decal.albedoMap);
            activeShader->setInt("u_DecalAlbedo", 0);
            activeShader->setBool("u_HasDecalTexture", decal.albedoMap != 0);
            dc.DrawElements(Primitive::Triangles, 6, DataType::UnsignedInt, 0);
        }
    }

    if (isDeferred)
    {
        geoSys->EndDecalPass(m_RenderService ? m_RenderService->GetMainFBO() : 0);
    }

    sm.SetColorMask(true, true, true, true);
    sm.SetDepthMask(true);
    sm.SetDepthFunc(CompareFunc::Less);
    sm.Disable(ServerCapability::Blend);
    sm.Enable(ServerCapability::CullFace);
    sm.SetCullFace(CullMode::Back);
}

std::vector<entt::id_type> DecalSystem::GetReadComponents() const
{
    return {entt::type_id<DecalComponent>().hash(), entt::type_id<PositionComponent>().hash(),
            entt::type_id<RotationComponent>().hash(), entt::type_id<ScaleComponent>().hash()};
}

std::vector<entt::id_type> DecalSystem::GetWriteComponents() const
{
    return {entt::type_id<DecalComponent>().hash()};
}

uint32_t DecalSystem::GetTagBit(const std::string& tag)
{
    if (tag.empty())
        return 1;  // Bit 0 is for default/untagged
    auto it = m_TagBitMap.find(tag);
    if (it != m_TagBitMap.end())
        return it->second;

    if (m_TagBitMap.size() >= 31)
    {
        LOGGER_WARN("DecalSystem") << "Too many decal tags; tag '" << tag << "' cannot be represented.";
        return 0;
    }
    uint32_t bit = 1u << (m_TagBitMap.size() + 1u);
    m_TagBitMap[tag] = bit;
    return bit;
}

uint32_t DecalSystem::GetBitmask(const std::vector<std::string>& tags)
{
    uint32_t mask = 0;
    for (const auto& tag : tags) mask |= GetTagBit(tag);
    return mask;
}

void DecalSystem::UpdateTagMap(Scene& scene)
{
    if (!m_GraphicsContext)
        return;
    auto& tm = m_GraphicsContext->GetTextureManager();
    if (m_TagMapTexture == 0)
        m_TagMapTexture = tm.GenTexture();

    auto view = scene.View<InfoComponent>();
    uint32_t maxEntityId = 0;
    for (auto entity : view) maxEntityId = std::max(maxEntityId, static_cast<uint32_t>(entt::to_entity(entity)));
    m_TagBuffer.assign(static_cast<size_t>(maxEntityId) + 1u, 0u);

    for (auto entity : view)
    {
        const uint32_t id = static_cast<uint32_t>(entt::to_entity(entity));
        auto& info = view.get<InfoComponent>(entity);
        m_TagBuffer[id] = GetTagBit(info.tag);
    }

    tm.BindTexture(TextureType::Texture1D, m_TagMapTexture);
    tm.TexImage1D(TextureType::Texture1D, 0, InternalFormat::R32UI, static_cast<int>(m_TagBuffer.size()), 0,
                  TextureFormat::Red_Integer, DataType::UnsignedInt, m_TagBuffer.data());
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
    tm.TexParameteri(TextureType::Texture1D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);
}
