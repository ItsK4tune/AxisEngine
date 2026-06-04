#include <ecs/logic/entity_builder.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <engine/ecs/unit/fragment_component.h>
#include <cmath>
#include <navigation/unit/pathfollower_component.h>
#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_rigid_body.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/animator.h>
#include <ecs/interface/i_script_registry.h>
#include <script/logic/scriptable.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene_manager.h>
#include <algorithm>

namespace
{
void ApplyUITransform(UITransformComponent& uiTransform, const glm::vec2& pos, const glm::vec2& size,
                      const glm::bvec2& positionIsPercent, const glm::bvec2& sizeIsPercent, int zIndex)
{
    uiTransform.position = pos;
    uiTransform.positionIsPercent = positionIsPercent;
    uiTransform.size = size;
    uiTransform.sizeIsPercent = sizeIsPercent;
    uiTransform.zIndex = zIndex;
    uiTransform.anchorMin = glm::vec2(0.0f);
    uiTransform.anchorMax = glm::vec2(0.0f);
    uiTransform.offsetMin = glm::vec2(0.0f);
    uiTransform.offsetMax = glm::vec2(0.0f);
}

void MarkWorldDirty(Scene& scene, entt::entity entity)
{
    auto& world = scene.registry.get_or_emplace<WorldTransformComponent>(entity);
    world.isDirty = true;
}

void BindMaterialTexture(AxisMaterialComponent& mat, MaterialTextureSlot slot, const std::string& textureNameOrPath,
                         ResourceManager& resources)
{
    uint32_t* gpuMap = nullptr;
    std::string* path = nullptr;

    switch (slot)
    {
        case MaterialTextureSlot::Albedo:
            path = &mat.desc.albedoPath;
            gpuMap = &mat.gpu.albedoMap;
            break;
        case MaterialTextureSlot::Normal:
            path = &mat.desc.normalPath;
            gpuMap = &mat.gpu.normalMap;
            break;
        case MaterialTextureSlot::Metallic:
            path = &mat.desc.metallicPath;
            gpuMap = &mat.gpu.metallicMap;
            break;
        case MaterialTextureSlot::Roughness:
            path = &mat.desc.roughnessPath;
            gpuMap = &mat.gpu.roughnessMap;
            break;
        case MaterialTextureSlot::AO:
            path = &mat.desc.aoPath;
            gpuMap = &mat.gpu.aoMap;
            break;
        case MaterialTextureSlot::Emissive:
            path = &mat.desc.emissivePath;
            gpuMap = &mat.gpu.emissiveMap;
            break;
        case MaterialTextureSlot::Specular:
            path = &mat.desc.specularPath;
            gpuMap = &mat.gpu.specularMap;
            break;
    }

    if (!path || !gpuMap)
        return;

    *path = textureNameOrPath;
    auto texture = resources.GetTextureAuto(textureNameOrPath);
    *gpuMap = texture ? texture->id : 0;
    mat.gpu.batchKeyDirty = true;
    mat.gpu.dirty = !texture;
}

namespace Perlin
{
    inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    inline float lerp(float t, float a, float b) { return a + t * (b - a); }
    inline float grad(int hash, float x, float y)
    {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
    
    inline int hashCoords(int x, int y)
    {
        unsigned int h = x * 374761393 + y * 668265263;
        h = (h ^ (h >> 13)) * 12741261;
        return h ^ (h >> 16);
    }

    inline float Noise2D(float x, float y)
    {
        int ix = (int)std::floor(x);
        int iy = (int)std::floor(y);
        float fx = x - ix;
        float fy = y - iy;

        float u = fade(fx);
        float v = fade(fy);

        int h00 = hashCoords(ix, iy);
        int h10 = hashCoords(ix + 1, iy);
        int h01 = hashCoords(ix, iy + 1);
        int h11 = hashCoords(ix + 1, iy + 1);

        float n00 = grad(h00, fx, fy);
        float n10 = grad(h10, fx - 1.0f, fy);
        float n01 = grad(h01, fx, fy - 1.0f);
        float n11 = grad(h11, fx - 1.0f, fy - 1.0f);

        float x1 = lerp(u, n00, n10);
        float x2 = lerp(u, n01, n11);

        return (lerp(v, x1, x2) + 1.0f) * 0.5f; // [0, 1]
    }
}
}  // namespace

EntityBuilder::EntityBuilder(Scene& scene, ResourceManager& resources, const std::string& sceneName)
    : m_Scene(scene), m_Resources(resources)
{
    m_Entity = m_Scene.registry.create();

    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.sceneName = sceneName;

    if (auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>())
    {
        sceneMgr->AddEntity(m_Entity, sceneName);
    }
}

EntityBuilder& EntityBuilder::WithTextureResource(const std::string& name, const std::string& path, bool async,
                                                  bool keepCpuData)
{
    if (!m_Resources.GetTexture(name))
        m_Resources.LoadTexture(name, path, async, keepCpuData);
    return *this;
}

EntityBuilder& EntityBuilder::WithModelResource(const std::string& name, const std::string& path, bool isStatic)
{
    if (!m_Resources.GetModel(name))
        m_Resources.LoadModel(name, path, isStatic);
    return *this;
}

EntityBuilder& EntityBuilder::WithShaderResource(const std::string& name, const std::string& vertexPath,
                                                 const std::string& fragmentPath, const std::string& geometryPath)
{
    if (!m_Resources.GetShader(name))
        m_Resources.LoadShader(name, vertexPath, fragmentPath, geometryPath);
    return *this;
}

EntityBuilder& EntityBuilder::WithFontResource(const std::string& name, const std::string& path, unsigned int fontSize)
{
    if (!m_Resources.GetFont(name))
        m_Resources.LoadFont(name, path, fontSize);
    return *this;
}

EntityBuilder& EntityBuilder::WithAnimationResource(const std::string& name, const std::string& path,
                                                    const std::string& modelName)
{
    if (!m_Resources.GetAnimation(name))
        m_Resources.LoadAnimation(name, path, modelName);
    return *this;
}

EntityBuilder& EntityBuilder::WithSkyboxResource(const std::string& name, const std::vector<std::string>& faces)
{
    if (!m_Resources.GetSkybox(name))
        m_Resources.LoadSkybox(name, faces);
    return *this;
}

EntityBuilder& EntityBuilder::WithName(const std::string& name)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.name = name;
    return *this;
}

EntityBuilder& EntityBuilder::WithTag(const std::string& tag)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.tag = tag;
    return *this;
}

EntityBuilder& EntityBuilder::WithLayer(uint32_t layer)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.layer = layer;
    return *this;
}

EntityBuilder& EntityBuilder::WithActive(bool active)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.isActive = active;
    return *this;
}

EntityBuilder& EntityBuilder::WithRenderOrder(int renderOrder)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    info.renderOrder = renderOrder;
    return *this;
}

EntityBuilder& EntityBuilder::WithScene(const std::string& sceneName)
{
    auto& info = m_Scene.registry.get_or_emplace<InfoComponent>(m_Entity);
    std::string oldScene = info.sceneName;
    info.sceneName = sceneName;

    if (auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>())
    {
        if (!oldScene.empty() && oldScene != sceneName)
        {
            sceneMgr->RemoveEntity(m_Entity);
        }
        sceneMgr->AddEntity(m_Entity, sceneName);
    }
    return *this;
}

EntityBuilder& EntityBuilder::WithTransform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
{
    m_Scene.registry.emplace_or_replace<PositionComponent>(m_Entity, pos, pos);
    m_Scene.registry.emplace_or_replace<RotationComponent>(m_Entity, glm::quat(glm::radians(rot)),
                                                           glm::quat(glm::radians(rot)));
    m_Scene.registry.emplace_or_replace<ScaleComponent>(m_Entity, scale, scale);
    MarkWorldDirty(m_Scene, m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithPosition(const glm::vec3& pos)
{
    m_Scene.registry.emplace_or_replace<PositionComponent>(m_Entity, pos, pos);
    MarkWorldDirty(m_Scene, m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithRotationEuler(const glm::vec3& rotDegrees)
{
    glm::quat rotation = glm::quat(glm::radians(rotDegrees));
    return WithRotation(rotation);
}

EntityBuilder& EntityBuilder::WithRotation(const glm::quat& rotation)
{
    m_Scene.registry.emplace_or_replace<RotationComponent>(m_Entity, rotation, rotation);
    MarkWorldDirty(m_Scene, m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithScale(const glm::vec3& scale)
{
    m_Scene.registry.emplace_or_replace<ScaleComponent>(m_Entity, scale, scale);
    MarkWorldDirty(m_Scene, m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithScale(float uniformScale)
{
    return WithScale(glm::vec3(uniformScale));
}

EntityBuilder& EntityBuilder::WithUIPosition(const glm::vec2& pos, const glm::bvec2& isPercent)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    uiTransform.position = pos;
    uiTransform.positionIsPercent = isPercent;
    return *this;
}

EntityBuilder& EntityBuilder::WithMesh(const std::string& modelName, const std::string& shaderName)
{
    auto& res = m_Resources;
    auto& mesh = m_Scene.registry.get_or_emplace<MeshRendererComponent>(m_Entity);
    mesh.model = res.GetModel(modelName);
    mesh.shader = res.GetShader(shaderName);
    mesh.shaderName = shaderName;
    return *this;
}

EntityBuilder& EntityBuilder::WithMeshAuto(const std::string& modelNameOrPath, const std::string& shaderName,
                                           bool isStatic)
{
    auto& mesh = m_Scene.registry.get_or_emplace<MeshRendererComponent>(m_Entity);
    mesh.model = m_Resources.GetModelAuto(modelNameOrPath, isStatic);
    mesh.shader = m_Resources.GetShader(shaderName);
    mesh.shaderName = shaderName;
    return *this;
}

EntityBuilder& EntityBuilder::WithMaterial(const AxisMaterialComponent& material)
{
    auto& mat = m_Scene.registry.emplace_or_replace<AxisMaterialComponent>(m_Entity, material);
    mat.gpu.batchKeyDirty = true;
    return *this;
}

EntityBuilder& EntityBuilder::WithPhongMaterial(const glm::vec3& ambient, const glm::vec3& specular, float shininess)
{
    // Legacy support: convert to PBR roughly
    auto& mat = m_Scene.registry.get_or_emplace<AxisMaterialComponent>(m_Entity);
    mat.desc.pbr.roughness = glm::clamp(1.0f - (shininess / 128.0f), 0.0f, 1.0f);
    mat.desc.pbr.metallic = 0.0f;
    mat.gpu.batchKeyDirty = true;
    return *this;
}

EntityBuilder& EntityBuilder::WithPBRMaterial(float metallic, float roughness, float ao)
{
    auto& mat = m_Scene.registry.get_or_emplace<AxisMaterialComponent>(m_Entity);
    mat.desc.pbr.metallic = metallic;
    mat.desc.pbr.roughness = roughness;
    mat.desc.pbr.ao = ao;
    mat.gpu.batchKeyDirty = true;
    return *this;
}

EntityBuilder& EntityBuilder::WithPBRMesh(const std::string& modelName, const std::string& shaderName, float metallic,
                                          float roughness, float ao)
{
    WithMesh(modelName, shaderName);
    return WithPBRMaterial(metallic, roughness, ao);
}

EntityBuilder& EntityBuilder::WithPBRRenderable(const std::string& modelName, const std::string& shaderName,
                                                const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale,
                                                float metallic, float roughness, float ao)
{
    WithTransform(pos, rot, scale);
    return WithPBRMesh(modelName, shaderName, metallic, roughness, ao);
}

EntityBuilder& EntityBuilder::WithPBRRenderable(const std::string& modelName, const std::string& shaderName,
                                                const glm::vec3& pos, const glm::vec3& rot, float uniformScale,
                                                float metallic, float roughness, float ao)
{
    return WithPBRRenderable(modelName, shaderName, pos, rot, glm::vec3(uniformScale), metallic, roughness, ao);
}

EntityBuilder& EntityBuilder::WithRendererColor(const glm::vec4& color)
{
    auto& mesh = m_Scene.registry.get_or_emplace<MeshRendererComponent>(m_Entity);
    mesh.color = color;
    return *this;
}

EntityBuilder& EntityBuilder::WithMeshRenderOptions(bool castShadow, bool receiveShadow, bool ignoreDepth, int order)
{
    auto& mesh = m_Scene.registry.get_or_emplace<MeshRendererComponent>(m_Entity);
    mesh.castShadow = castShadow;
    mesh.receiveShadow = receiveShadow;
    mesh.ignoreDepth = ignoreDepth;
    mesh.order = order;
    return *this;
}

EntityBuilder& EntityBuilder::WithLOD(const std::vector<std::string>& modelNamesOrPaths,
                                      const std::vector<float>& distances, bool distancesAreSquared,
                                      bool isStaticModel)
{
    auto& lod = m_Scene.registry.get_or_emplace<LODComponent>(m_Entity);
    lod.lodModels.clear();
    lod.lodDistancesSq.clear();

    const size_t pairCount = std::min(modelNamesOrPaths.size(), distances.size());
    lod.lodModels.reserve(pairCount);
    lod.lodDistancesSq.reserve(pairCount);

    for (size_t i = 0; i < pairCount; ++i)
    {
        lod.lodModels.push_back(m_Resources.GetModelAuto(modelNamesOrPaths[i], isStaticModel));

        const float distance = glm::max(distances[i], 0.0f);
        lod.lodDistancesSq.push_back(distancesAreSquared ? distance : distance * distance);
    }

    return *this;
}

EntityBuilder& EntityBuilder::WithLODModel(const std::string& modelNameOrPath, float distance,
                                           bool distanceIsSquared, bool isStaticModel)
{
    auto& lod = m_Scene.registry.get_or_emplace<LODComponent>(m_Entity);
    lod.lodModels.push_back(m_Resources.GetModelAuto(modelNameOrPath, isStaticModel));

    const float clampedDistance = glm::max(distance, 0.0f);
    lod.lodDistancesSq.push_back(distanceIsSquared ? clampedDistance : clampedDistance * clampedDistance);
    return *this;
}

EntityBuilder& EntityBuilder::WithOcclusion(bool visible)
{
    auto& occlusion = m_Scene.registry.get_or_emplace<OcclusionComponent>(m_Entity);
    occlusion.isVisible = visible;
    occlusion.queryPending = false;
    return *this;
}

EntityBuilder& EntityBuilder::WithStreaming(const std::string& modelPath, float loadDistance, float unloadDistance,
                                            bool isStatic)
{
    auto& streaming = m_Scene.registry.get_or_emplace<StreamingComponent>(m_Entity);
    streaming.modelPath = modelPath;
    streaming.loadDistance = glm::max(loadDistance, 0.0f);
    streaming.unloadDistance = glm::max(unloadDistance, streaming.loadDistance);
    streaming.isStatic = isStatic;
    streaming.isRequested = false;
    return *this;
}

EntityBuilder& EntityBuilder::WithMaterialEmission(const glm::vec3& emission)
{
    auto& mat = m_Scene.registry.get_or_emplace<AxisMaterialComponent>(m_Entity);
    mat.desc.emission = emission;
    mat.gpu.batchKeyDirty = true;
    mat.gpu.dirty = true;
    return *this;
}

EntityBuilder& EntityBuilder::WithMaterialTexture(MaterialTextureSlot slot, const std::string& textureNameOrPath)
{
    auto& mat = m_Scene.registry.get_or_emplace<AxisMaterialComponent>(m_Entity);
    BindMaterialTexture(mat, slot, textureNameOrPath, m_Resources);
    return *this;
}

EntityBuilder& EntityBuilder::WithMaterialTextureResource(MaterialTextureSlot slot, const std::string& textureName,
                                                          const std::string& path, bool async, bool keepCpuData)
{
    WithTextureResource(textureName, path, async, keepCpuData);
    return WithMaterialTexture(slot, textureName);
}

EntityBuilder& EntityBuilder::WithMaterialTextures(const std::string& albedo, const std::string& normal,
                                                   const std::string& metallic, const std::string& roughness,
                                                   const std::string& ao, const std::string& emissive,
                                                   const std::string& specular)
{
    if (!albedo.empty())
        WithMaterialTexture(MaterialTextureSlot::Albedo, albedo);
    if (!normal.empty())
        WithMaterialTexture(MaterialTextureSlot::Normal, normal);
    if (!metallic.empty())
        WithMaterialTexture(MaterialTextureSlot::Metallic, metallic);
    if (!roughness.empty())
        WithMaterialTexture(MaterialTextureSlot::Roughness, roughness);
    if (!ao.empty())
        WithMaterialTexture(MaterialTextureSlot::AO, ao);
    if (!emissive.empty())
        WithMaterialTexture(MaterialTextureSlot::Emissive, emissive);
    if (!specular.empty())
        WithMaterialTexture(MaterialTextureSlot::Specular, specular);
    return *this;
}

EntityBuilder& EntityBuilder::WithRigidBody(std::shared_ptr<IRigidBody> body)
{
    auto& rb = m_Scene.registry.get_or_emplace<RigidBodyComponent>(m_Entity);
    rb.body = body;
    return *this;
}

EntityBuilder& EntityBuilder::WithCharacterController(std::shared_ptr<ICharacterController> controller)
{
    auto& cc = m_Scene.registry.get_or_emplace<CharacterControllerComponent>(m_Entity);
    cc.controller = controller;
    return *this;
}

EntityBuilder& EntityBuilder::WithPathFollower(float moveSpeed, float rotationSpeed, float maxRotationSpeed,
                                               float rotationAcceleration, const glm::vec3& rotationOffset)
{
    auto& follower = m_Scene.registry.get_or_emplace<PathFollowerComponent>(m_Entity);
    follower.moveSpeed = moveSpeed;
    follower.rotationSpeed = rotationSpeed;
    follower.maxRotationSpeed = maxRotationSpeed;
    follower.rotationAcceleration = rotationAcceleration;
    follower.rotationOffset = rotationOffset;
    return *this;
}

EntityBuilder& EntityBuilder::WithUITransform(const glm::vec2& pos, const glm::vec2& size, int zIndex)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ApplyUITransform(uiTransform, pos, size, glm::bvec2(false), glm::bvec2(false), zIndex);
    return *this;
}

EntityBuilder& EntityBuilder::WithUITransform(const glm::vec2& pos, const glm::vec2& size,
                                              const glm::bvec2& positionIsPercent,
                                              const glm::bvec2& sizeIsPercent, int zIndex)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ApplyUITransform(uiTransform, pos, size, positionIsPercent, sizeIsPercent, zIndex);
    return *this;
}

EntityBuilder& EntityBuilder::WithUITransformPercent(const glm::vec2& posPercent, const glm::vec2& sizePercent,
                                                     int zIndex)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ApplyUITransform(uiTransform, posPercent, sizePercent, glm::bvec2(true), glm::bvec2(true), zIndex);
    return *this;
}

EntityBuilder& EntityBuilder::WithUITransformPercentPosition(const glm::vec2& posPercent, const glm::vec2& size,
                                                             int zIndex)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ApplyUITransform(uiTransform, posPercent, size, glm::bvec2(true), glm::bvec2(false), zIndex);
    return *this;
}

EntityBuilder& EntityBuilder::WithUIAnchored(const glm::vec2& anchor, const glm::vec2& pos, const glm::vec2& size,
                                             int zIndex)
{
    WithUITransform(pos, size, zIndex);
    return WithUIAnchors(anchor, anchor);
}

EntityBuilder& EntityBuilder::WithUIAnchoredChild(entt::entity parent, const glm::vec2& anchor, const glm::vec2& pos,
                                                  const glm::vec2& size, int zIndex)
{
    WithParent(parent);
    return WithUIAnchored(anchor, pos, size, zIndex);
}

EntityBuilder& EntityBuilder::WithUIChild(entt::entity parent, const glm::vec2& pos, const glm::vec2& size, int zIndex)
{
    WithParent(parent);
    return WithUITransform(pos, size, zIndex);
}

EntityBuilder& EntityBuilder::WithUIStretchChild(entt::entity parent, const glm::vec2& anchorMin,
                                                 const glm::vec2& anchorMax, const glm::vec2& offsetMin,
                                                 const glm::vec2& offsetMax, int zIndex)
{
    WithParent(parent);
    WithUIStretch(anchorMin, anchorMax, offsetMin, offsetMax);
    return WithUIZIndex(zIndex);
}

EntityBuilder& EntityBuilder::WithUIPositionPercent(const glm::vec2& posPercent)
{
    return WithUIPosition(posPercent, glm::bvec2(true));
}

EntityBuilder& EntityBuilder::WithUISize(const glm::vec2& size, const glm::bvec2& isPercent)
{
    auto& uiTransform = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    uiTransform.size = size;
    uiTransform.sizeIsPercent = isPercent;
    return *this;
}

EntityBuilder& EntityBuilder::WithUISizePercent(const glm::vec2& sizePercent)
{
    return WithUISize(sizePercent, glm::bvec2(true));
}

EntityBuilder& EntityBuilder::WithUIText(const std::string& text, const std::string& fontName, float scale,
                                         const glm::vec4& color)
{
    auto& res = m_Resources;
    auto& textComp = m_Scene.registry.get_or_emplace<UITextComponent>(m_Entity);

    textComp.text = text;
    textComp.fontName = fontName;
    textComp.scale = scale;
    textComp.color = color;

    textComp.font = res.GetFont(fontName);
    textComp.shader = res.GetShader("textShader");

    std::string uniqueModelName = "ui_text_model_" + std::to_string((uint32_t)m_Entity);
    if (!res.HasUIModel(uniqueModelName))
    {
        res.CreateUIModel(uniqueModelName, ::UIType::Text);
    }
    textComp.model = res.GetUIModel(uniqueModelName);

    return *this;
}

EntityBuilder& EntityBuilder::WithUIAnchors(const glm::vec2& min, const glm::vec2& max)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.anchorMin = min;
    ui.anchorMax = max;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIOffsets(const glm::vec2& min, const glm::vec2& max)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.offsetMin = min;
    ui.offsetMax = max;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIStretch(const glm::vec2& anchorMin, const glm::vec2& anchorMax,
                                            const glm::vec2& offsetMin, const glm::vec2& offsetMax)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.position = glm::vec2(0.0f);
    ui.positionIsPercent = glm::bvec2(false);
    ui.size = glm::vec2(0.0f);
    ui.sizeIsPercent = glm::bvec2(false);
    ui.anchorMin = anchorMin;
    ui.anchorMax = anchorMax;
    ui.offsetMin = offsetMin;
    ui.offsetMax = offsetMax;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIFillParent(int zIndex)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.zIndex = zIndex;
    return WithUIStretch(glm::vec2(0.0f), glm::vec2(1.0f));
}

EntityBuilder& EntityBuilder::WithUIPivot(const glm::vec2& pivot)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.pivot = pivot;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIFlip(bool flipX, bool flipY)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.flipX = flipX;
    ui.flipY = flipY;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIRotation(float degrees)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.rotation = degrees;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIZIndex(int zIndex)
{
    auto& ui = m_Scene.registry.get_or_emplace<UITransformComponent>(m_Entity);
    ui.zIndex = zIndex;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIFlex(FlexDirection dir, float spacing)
{
    auto& flex = m_Scene.registry.get_or_emplace<UIFlexLayoutComponent>(m_Entity);
    flex.direction = dir;
    flex.spacing = spacing;
    return *this;
}

EntityBuilder& EntityBuilder::WithUITextAlignment(TextAlignment align, bool wrap, float maxWidth)
{
    auto& text = m_Scene.registry.get_or_emplace<UITextComponent>(m_Entity);
    text.alignment = align;
    text.wordWrap = wrap;
    text.maxWidth = maxWidth;
    return *this;
}

EntityBuilder& EntityBuilder::WithUIRenderer(const std::string& textureName, const glm::vec4& color)
{
    auto& res = m_Resources;
    auto& renderer = m_Scene.registry.get_or_emplace<UIRendererComponent>(m_Entity);

    renderer.color = color;
    renderer.shader = res.GetShader("uiShader");

    if (!res.GetUIModel(textureName))
    {
        res.CreateUIModel(textureName, ::UIType::Texture);
    }
    renderer.model = res.GetUIModel(textureName);

    return *this;
}

EntityBuilder& EntityBuilder::WithUITexture(const std::string& textureNameOrPath, const glm::vec4& color,
                                            const std::string& uiModelName)
{
    const std::string modelName =
        uiModelName.empty() ? ("ui_texture_" + std::to_string(static_cast<uint32_t>(m_Entity))) : uiModelName;
    WithUIRenderer(modelName, color);

    auto texture = m_Resources.GetTextureAuto(textureNameOrPath);
    auto& renderer = m_Scene.registry.get_or_emplace<UIRendererComponent>(m_Entity);
    renderer.texture = texture;
    if (renderer.model && texture)
        renderer.model->SetTexture(texture->id);

    return *this;
}

EntityBuilder& EntityBuilder::WithUITextureResource(const std::string& textureName, const std::string& path,
                                                    const glm::vec4& color, const std::string& uiModelName,
                                                    bool async, bool keepCpuData)
{
    WithTextureResource(textureName, path, async, keepCpuData);
    return WithUITexture(textureName, color, uiModelName.empty() ? textureName : uiModelName);
}

EntityBuilder& EntityBuilder::WithParent(entt::entity parent)
{
    if (parent == entt::null)
        return *this;

    auto& hierarchy = m_Scene.registry.get_or_emplace<HierarchyComponent>(m_Entity);
    (void)hierarchy;
    if (m_Scene.registry.valid(parent))
    {
        auto& parentHierarchy = m_Scene.registry.get_or_emplace<HierarchyComponent>(parent);
        (void)parentHierarchy;
    }

    EntityManager::SetParent(m_Scene, m_Entity, parent);
    return *this;
}

EntityBuilder& EntityBuilder::WithAudio(const std::string& soundName, bool loop, float volume)
{
    auto& res = m_Resources;
    auto& audio = m_Scene.registry.get_or_emplace<AudioSourceComponent>(m_Entity);
    audio.sound = std::dynamic_pointer_cast<ISound>(res.GetSound(soundName));
    if (audio.sound)
    {
        audio.loop = loop;
        audio.volume = volume;
    }
    return *this;
}

EntityBuilder& EntityBuilder::WithScript(const std::string& scriptName)
{
    auto& script = m_Scene.registry.get_or_emplace<ScriptComponent>(m_Entity);
    script.className = scriptName;
    script.instance.reset();
    script.scriptableInstance = nullptr;
    script.inputScriptableInstance = nullptr;

    if (scriptName.empty() || scriptName == "None")
    {
        script.InstantiateScript = nullptr;
        script.DestroyScript = nullptr;
        return *this;
    }

    script.InstantiateScript = [scriptName]() {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(scriptName) : nullptr;
    };
    script.DestroyScript = [](ScriptComponent* sc) {
        sc->instance.reset();
        sc->scriptableInstance = nullptr;
        sc->inputScriptableInstance = nullptr;
    };
    return *this;
}

EntityBuilder& EntityBuilder::WithAnimation(const std::string& animationName)
{
    auto& res = m_Resources;
    auto& anim = m_Scene.registry.get_or_emplace<AnimationComponent>(m_Entity);
    anim.animations.push_back(animationName);

    auto a = res.GetAnimation(animationName);
    if (a)
    {
        if (!anim.animator)
        {
            anim.animator = std::make_shared<Animator>(a);
            anim.animator->AddAnimation(animationName, a);
        }
        else
        {
            anim.animator->AddAnimation(animationName, a);
        }
    }
    return *this;
}

EntityBuilder& EntityBuilder::WithDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    auto& light = m_Scene.registry.get_or_emplace<DirectionalLightComponent>(m_Entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    return *this;
}

EntityBuilder& EntityBuilder::WithDirectionalLightAt(const glm::vec3& pos, const glm::vec3& rot,
                                                     const glm::vec3& direction, const glm::vec3& color,
                                                     float intensity, const glm::vec3& scale)
{
    WithTransform(pos, rot, scale);
    return WithDirectionalLight(direction, color, intensity);
}

EntityBuilder& EntityBuilder::WithPointLight(const glm::vec3& color, float intensity, float radius)
{
    auto& light = m_Scene.registry.get_or_emplace<PointLightComponent>(m_Entity);
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    return *this;
}

EntityBuilder& EntityBuilder::WithPointLightAt(const glm::vec3& pos, const glm::vec3& color, float intensity,
                                               float radius)
{
    WithPosition(pos);
    return WithPointLight(color, intensity, radius);
}

EntityBuilder& EntityBuilder::WithSpotLight(const glm::vec3& direction, const glm::vec3& color, float intensity,
                                            float radius)
{
    auto& light = m_Scene.registry.get_or_emplace<SpotLightComponent>(m_Entity);
    light.direction = direction;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    return *this;
}

EntityBuilder& EntityBuilder::WithSpotLightAt(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& direction,
                                              const glm::vec3& color, float intensity, float radius)
{
    WithTransform(pos, rot);
    return WithSpotLight(direction, color, intensity, radius);
}

EntityBuilder& EntityBuilder::WithCamera(float fov, float near, float far, bool active)
{
    auto& cam = m_Scene.registry.get_or_emplace<CameraComponent>(m_Entity);
    cam.fov = fov;
    cam.nearPlane = near;
    cam.farPlane = far;
    if (active)
        EntityManager::SetActiveCamera(m_Scene, m_Entity);
    return *this;
}

EntityBuilder& EntityBuilder::WithParticle(const std::string& textureName)
{
    auto& p = m_Scene.registry.get_or_emplace<ParticleEmitterComponent>(m_Entity);
    p.textureName = textureName;
    return *this;
}

EntityBuilder& EntityBuilder::WithParticleTexture(const std::string& textureNameOrPath)
{
    auto& p = m_Scene.registry.get_or_emplace<ParticleEmitterComponent>(m_Entity);
    p.textureName = textureNameOrPath;
    p.emitter.Texture = m_Resources.GetTextureAuto(textureNameOrPath);
    return *this;
}

EntityBuilder& EntityBuilder::WithParticleTextureResource(const std::string& textureName, const std::string& path,
                                                          bool async, bool keepCpuData)
{
    WithTextureResource(textureName, path, async, keepCpuData);
    return WithParticleTexture(textureName);
}

EntityBuilder& EntityBuilder::WithVideo(const std::string& videoPath, bool loop)
{
    auto& video = m_Scene.registry.get_or_emplace<VideoPlayerComponent>(m_Entity);
    video.filePath = videoPath;
    video.isLooping = loop;
    return *this;
}

EntityBuilder& EntityBuilder::WithVideoPlayback(bool playOnAwake, bool isPlaying, float volume, float speed,
                                                int maxDecodes)
{
    auto& video = m_Scene.registry.get_or_emplace<VideoPlayerComponent>(m_Entity);
    video.playOnAwake = playOnAwake;
    video.isPlaying = isPlaying;
    video.volume = volume;
    video.speed = speed;
    video.maxDecodes = maxDecodes;
    return *this;
}

EntityBuilder& EntityBuilder::WithPlayingVideo(const std::string& videoPath, bool loop, float volume, int maxDecodes,
                                               float speed)
{
    WithVideo(videoPath, loop);
    return WithVideoPlayback(true, true, volume, speed, maxDecodes);
}

EntityBuilder& EntityBuilder::WithUIVideo(const std::string& videoPath, const std::string& uiModelName,
                                          const glm::vec4& color, bool loop, float volume, int maxDecodes,
                                          float speed)
{
    WithUIRenderer(uiModelName, color);
    return WithPlayingVideo(videoPath, loop, volume, maxDecodes, speed);
}

EntityBuilder& EntityBuilder::WithTerrain(const glm::vec3& terrainSize, float maxHeight, int resolution, int chunkSize,
                                           float textureScale, const std::string& heightMapName,
                                           const std::string& splatMapName,
                                           const std::vector<std::string>& diffuseLayerNames,
                                           bool generatePhysics, bool castShadows)
{
    auto& terrain = m_Scene.registry.get_or_emplace<TerrainComponent>(m_Entity);
    terrain.terrainSize = terrainSize;
    terrain.maxHeight = maxHeight;
    terrain.resolution = resolution;
    terrain.chunkSize = chunkSize;
    terrain.textureScale = textureScale;
    terrain.generatePhysics = generatePhysics;
    terrain.castShadows = castShadows;
    terrain.needsRebuild = true;
    terrain.isWalkable = true;

    auto heightTex = m_Resources.GetTexture(heightMapName);
    if (heightTex)
    {
        terrain.heightMap = heightTex->id;
    }
    terrain.heightMapName = heightMapName;

    if (!splatMapName.empty())
    {
        auto splatTex = m_Resources.GetTexture(splatMapName);
        if (splatTex)
        {
            terrain.splatMap = splatTex->id;
        }
    }

    terrain.diffuseLayers.clear();
    for (const auto& layerName : diffuseLayerNames)
    {
        auto tex = m_Resources.GetTexture(layerName);
        if (tex)
        {
            terrain.diffuseLayers.push_back(tex->id);
        }
    }

    return *this;
}

entt::entity EntityBuilder::SpawnObject(Scene& scene, ResourceManager& res, const std::string& sceneName,
                                        const std::string& fragmentPath, const glm::vec3& pos,
                                        const glm::vec3& scale)
{
    auto e = EntityBuilder(scene, res, sceneName)
        .WithName("ProceduralObject")
        .WithTransform(pos, glm::vec3(0.0f, rand() % 360, 0.0f), scale)
        .Build();
        
    auto& frag = scene.registry.emplace<FragmentComponent>(e);
    frag.path = fragmentPath;
    frag.instantiated = false;
    
    return e;
}

void EntityBuilder::ScatterObjects(Scene& scene, ResourceManager& res, const std::string& sceneName,
                                   const std::string& fragmentPath, const PlacementRule& rule,
                                   const std::vector<float>& heights, int width, int height,
                                   float terrainWidth, float terrainLength, float terrainHeight,
                                   float waterLevel, float randomOffsetX, float randomOffsetZ,
                                   int attempts, const glm::vec3& scale)
{
    auto getTerrainHeight = [&](float wx, float wz) -> float {
        float localX = wx - (-terrainWidth * 0.5f);
        float localZ = wz - (-terrainLength * 0.5f);
        float gridXf = (localX / terrainWidth) * 256.0f;
        float gridZf = (localZ / terrainLength) * 256.0f;
        int gx = (std::max)(0, (std::min)(256, (int)std::round(gridXf)));
        int gz = (std::max)(0, (std::min)(256, (int)std::round(gridZf)));
        return heights[gz * width + gx] * terrainHeight;
    };

    auto getTerrainSlope = [&](float wx, float wz) -> float {
        float hL = getTerrainHeight(wx - 1.0f, wz);
        float hR = getTerrainHeight(wx + 1.0f, wz);
        float hD = getTerrainHeight(wx, wz - 1.0f);
        float hU = getTerrainHeight(wx, wz + 1.0f);
        return std::sqrt((hR - hL) * (hR - hL) + (hU - hD) * (hU - hD));
    };

    for (int i = 0; i < attempts; ++i)
    {
        float rx = (static_cast<float>(rand() % 1000) / 1000.0f - 0.5f) * (terrainWidth * 0.9f);
        float rz = (static_cast<float>(rand() % 1000) / 1000.0f - 0.5f) * (terrainLength * 0.9f);

        float h = getTerrainHeight(rx, rz);
        float slope = getTerrainSlope(rx, rz);

        if (h < rule.minHeight || h > rule.maxHeight || slope > rule.maxSlope)
            continue;

        float localX = rx - (-terrainWidth * 0.5f);
        float localZ = rz - (-terrainLength * 0.5f);
        float gridX = (localX / terrainWidth) * 256.0f;
        float gridZ = (localZ / terrainLength) * 256.0f;

        float riverNoise = Perlin::Noise2D(gridX * 0.008f + randomOffsetX + 500.0f, gridZ * 0.008f + randomOffsetZ + 500.0f);
        float riverFactor = std::abs(riverNoise - 0.5f);
        bool nearWater = (riverFactor < 0.08f) || (h < (waterLevel + 0.06f) * terrainHeight);

        float weight = 1.0f;
        if (nearWater) weight *= rule.waterWeight;
        if (h > 0.62f * terrainHeight || slope > 0.25f) weight *= rule.mountainWeight;
        if (h >= (waterLevel + 0.05f) * terrainHeight && h < 0.55f * terrainHeight && slope < 0.12f) weight *= rule.plainsWeight;

        if ((rand() % 100) < (weight * rule.baseProbability))
        {
            SpawnObject(scene, res, sceneName, fragmentPath, glm::vec3(rx, h, rz), scale);
        }
    }
}

entt::entity EntityBuilder::Build()
{
    return m_Entity;
}
