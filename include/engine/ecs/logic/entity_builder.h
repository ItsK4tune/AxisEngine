#pragma once

#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/network_components.h>
#include <navigation/unit/navmesh_component.h>
#include <scene/logic/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

class ResourceManager;
class IRigidBody;
class ICharacterController;

#define GLM_ENABLE_EXPERIMENTAL

enum class MaterialTextureSlot
{
    Albedo,
    Normal,
    Metallic,
    Roughness,
    AO,
    Emissive,
    Specular
};

struct PlacementRule
{
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
    float maxSlope = 1.0f;
    float waterWeight = 1.0f;      // weight factor near water
    float mountainWeight = 1.0f;   // weight factor on mountains
    float plainsWeight = 1.0f;     // weight factor on plains
    float baseProbability = 5.0f;  // base probability in percent
};

class EntityBuilder
{
public:
    EntityBuilder(Scene& scene, ResourceManager& resources, const std::string& sceneName);
    EntityBuilder(Scene& scene, ResourceManager& resources, entt::entity entity);

    EntityBuilder& WithTextureResource(const std::string& name, const std::string& path, bool async = true,
                                       bool keepCpuData = false);
    EntityBuilder& WithModelResource(const std::string& name, const std::string& path, bool isStatic = false);
    EntityBuilder& WithShaderResource(const std::string& name, const std::string& vertexPath,
                                      const std::string& fragmentPath, const std::string& geometryPath = "");
    EntityBuilder& WithFontResource(const std::string& name, const std::string& path, unsigned int fontSize);
    EntityBuilder& WithAnimationResource(const std::string& name, const std::string& path,
                                         const std::string& modelName);
    EntityBuilder& WithSkyboxResource(const std::string& name, const std::vector<std::string>& faces);

    EntityBuilder& WithName(const std::string& name);
    EntityBuilder& WithTag(const std::string& tag);
    EntityBuilder& WithLayer(uint32_t layer);
    EntityBuilder& WithActive(bool active);
    EntityBuilder& WithRenderOrder(int renderOrder);
    EntityBuilder& WithScene(const std::string& sceneName);
    EntityBuilder& WithTransient(bool transient = true);

    EntityBuilder& WithTransform(const glm::vec3& pos = glm::vec3(0.0f), const glm::vec3& rot = glm::vec3(0.0f),
                                 const glm::vec3& scale = glm::vec3(1.0f));
    EntityBuilder& WithPosition(const glm::vec3& pos);
    EntityBuilder& WithRotationEuler(const glm::vec3& rotDegrees);
    EntityBuilder& WithRotation(const glm::quat& rotation);
    EntityBuilder& WithScale(const glm::vec3& scale);
    EntityBuilder& WithScale(float uniformScale);

    EntityBuilder& WithMesh(const std::string& modelName, const std::string& shaderName);
    EntityBuilder& WithMeshAuto(const std::string& modelNameOrPath, const std::string& shaderName,
                                bool isStatic = false);
    EntityBuilder& WithMaterial(const MaterialComponent& material);
    EntityBuilder& WithPhongMaterial(const glm::vec3& ambient = glm::vec3(1.0f),
                                     const glm::vec3& specular = glm::vec3(0.5f), float shininess = 32.0f);
    EntityBuilder& WithPBRMaterial(float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);
    EntityBuilder& WithPBRMesh(const std::string& modelName, const std::string& shaderName, float metallic = 0.0f,
                               float roughness = 0.5f, float ao = 1.0f);
    EntityBuilder& WithPBRRenderable(const std::string& modelName, const std::string& shaderName, const glm::vec3& pos,
                                     const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f),
                                     float metallic = 0.0f, float roughness = 0.5f, float ao = 1.0f);
    EntityBuilder& WithPBRRenderable(const std::string& modelName, const std::string& shaderName, const glm::vec3& pos,
                                     const glm::vec3& rot, float uniformScale, float metallic = 0.0f,
                                     float roughness = 0.5f, float ao = 1.0f);
    EntityBuilder& WithRendererColor(const glm::vec4& color);
    EntityBuilder& WithMeshRenderOptions(bool castShadow, bool receiveShadow, bool ignoreDepth = false, int order = 0);
    EntityBuilder& WithLOD(const std::vector<std::string>& modelNamesOrPaths, const std::vector<float>& distances,
                           bool distancesAreSquared = false, bool isStaticModel = false);
    EntityBuilder& WithLODModel(const std::string& modelNameOrPath, float distance, bool distanceIsSquared = false,
                                bool isStaticModel = false);
    EntityBuilder& WithOcclusion(bool visible = true);
    EntityBuilder& WithStreaming(const std::string& modelPath, float loadDistance = 100.0f,
                                 float unloadDistance = 150.0f, bool isStatic = false);
    EntityBuilder& WithMaterialEmission(const glm::vec3& emission);
    EntityBuilder& WithMaterialTexture(MaterialTextureSlot slot, const std::string& textureNameOrPath);
    EntityBuilder& WithMaterialTextureResource(MaterialTextureSlot slot, const std::string& textureName,
                                               const std::string& path, bool async = true, bool keepCpuData = false);
    EntityBuilder& WithMaterialTextures(const std::string& albedo = "", const std::string& normal = "",
                                        const std::string& metallic = "", const std::string& roughness = "",
                                        const std::string& ao = "", const std::string& emissive = "",
                                        const std::string& specular = "");

    EntityBuilder& WithTerrain(const glm::vec3& terrainSize, float maxHeight, int resolution, int chunkSize,
                               float textureScale, const std::string& heightMapName,
                               const std::string& splatMapName = "",
                               const std::vector<std::string>& diffuseLayerNames = {}, bool generatePhysics = true,
                               bool castShadows = true);
    EntityBuilder& WithTerrain(const glm::vec3& terrainSize, float maxHeight = 0.0f, bool isWalkable = true,
                               bool generatePhysics = false);

    EntityBuilder& WithRigidBody(std::shared_ptr<IRigidBody> body);
    EntityBuilder& WithRigidBody(float mass = 1.0f, bool isStatic = false, bool isTrigger = false,
                                 float linearDamping = 0.0f, float angularDamping = 0.0f);
    EntityBuilder& WithCharacterController(std::shared_ptr<ICharacterController> controller, float stepHeight = 0.35f,
                                           float maxSlope = 45.0f);

    EntityBuilder& WithPathFollower(float moveSpeed = 5.0f, float rotationSpeed = 10.0f, float maxRotationSpeed = 20.0f,
                                    float rotationAcceleration = 40.0f,
                                    const glm::vec3& rotationOffset = glm::vec3(0.0f));

    EntityBuilder& WithUITransform(const glm::vec2& pos, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUITransform(const glm::vec2& pos, const glm::vec2& size, const glm::bvec2& positionIsPercent,
                                   const glm::bvec2& sizeIsPercent, int zIndex = 0);
    EntityBuilder& WithUITransformPercent(const glm::vec2& posPercent, const glm::vec2& sizePercent, int zIndex = 0);
    EntityBuilder& WithUITransformPercentPosition(const glm::vec2& posPercent, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIAnchored(const glm::vec2& anchor, const glm::vec2& pos, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIAnchoredChild(entt::entity parent, const glm::vec2& anchor, const glm::vec2& pos,
                                       const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIChild(entt::entity parent, const glm::vec2& pos, const glm::vec2& size, int zIndex = 0);
    EntityBuilder& WithUIStretchChild(entt::entity parent, const glm::vec2& anchorMin, const glm::vec2& anchorMax,
                                      const glm::vec2& offsetMin = glm::vec2(0.0f),
                                      const glm::vec2& offsetMax = glm::vec2(0.0f), int zIndex = 0);
    EntityBuilder& WithUIPosition(const glm::vec2& pos, const glm::bvec2& isPercent = glm::bvec2(false));
    EntityBuilder& WithUIPositionPercent(const glm::vec2& posPercent);
    EntityBuilder& WithUISize(const glm::vec2& size, const glm::bvec2& isPercent = glm::bvec2(false));
    EntityBuilder& WithUISizePercent(const glm::vec2& sizePercent);
    EntityBuilder& WithUIAnchors(const glm::vec2& min, const glm::vec2& max);
    EntityBuilder& WithUIOffsets(const glm::vec2& min, const glm::vec2& max);
    EntityBuilder& WithUIStretch(const glm::vec2& anchorMin, const glm::vec2& anchorMax,
                                 const glm::vec2& offsetMin = glm::vec2(0.0f),
                                 const glm::vec2& offsetMax = glm::vec2(0.0f));
    EntityBuilder& WithUIFillParent(int zIndex = 0);
    EntityBuilder& WithUIPivot(const glm::vec2& pivot);
    EntityBuilder& WithUIFlip(bool flipX, bool flipY);
    EntityBuilder& WithUIRotation(float degrees);
    EntityBuilder& WithUIZIndex(int zIndex);
    EntityBuilder& WithUIFlex(FlexDirection dir, float spacing = 5.0f);
    EntityBuilder& WithUIText(const std::string& text, const std::string& fontName, float scale = 1.0f,
                              const glm::vec4& color = glm::vec4(1.0f));
    EntityBuilder& WithUITextAlignment(TextAlignment align, bool wrap = false, float maxWidth = 0.0f);
    EntityBuilder& WithUIRenderer(const std::string& textureName, const glm::vec4& color = glm::vec4(1.0f));
    EntityBuilder& WithUITexture(const std::string& textureNameOrPath, const glm::vec4& color = glm::vec4(1.0f),
                                 const std::string& uiModelName = "");
    EntityBuilder& WithUITextureResource(const std::string& textureName, const std::string& path,
                                         const glm::vec4& color = glm::vec4(1.0f), const std::string& uiModelName = "",
                                         bool async = false, bool keepCpuData = true);
    EntityBuilder& WithParent(entt::entity parent);
    EntityBuilder& WithAudio(const std::string& soundName, bool loop = false, float volume = 100.0f);
    EntityBuilder& WithScript(const std::string& scriptName);
    EntityBuilder& WithScriptable(const std::string& className,
                                  std::function<std::unique_ptr<IScriptable>()> instantiateFunc);
    EntityBuilder& WithAnimation(const std::string& animationName);
    EntityBuilder& WithFragment(const std::string& path, const std::string& overrides = "");
    EntityBuilder& WithNetwork(uint32_t networkId = 0, uint32_t ownerId = 0, bool isLocal = false);

    EntityBuilder& WithDirectionalLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f),
                                        float intensity = 1.0f);
    EntityBuilder& WithDirectionalLightAt(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& direction,
                                          const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f,
                                          const glm::vec3& scale = glm::vec3(1.0f));
    EntityBuilder& WithPointLight(const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f,
                                  float radius = 10.0f);
    EntityBuilder& WithPointLightAt(const glm::vec3& pos, const glm::vec3& color = glm::vec3(1.0f),
                                    float intensity = 1.0f, float radius = 10.0f);
    EntityBuilder& WithSpotLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f),
                                 float intensity = 1.0f, float radius = 50.0f);
    EntityBuilder& WithSpotLightAt(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& direction,
                                   const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f,
                                   float radius = 50.0f);

    EntityBuilder& WithCamera(float fov = 45.0f, float near = 0.1f, float far = 1000.0f, bool active = false);

    EntityBuilder& WithParticle(const std::string& particleName);
    EntityBuilder& WithParticleTexture(const std::string& textureNameOrPath);
    EntityBuilder& WithParticleTextureResource(const std::string& textureName, const std::string& path,
                                               bool async = true, bool keepCpuData = false);
    EntityBuilder& WithParticleEmitter(float spawnRate = 10.0f, float lifeTime = 2.0f, float startSize = 0.1f,
                                       float endSize = 0.1f, const glm::vec3& minVelocity = glm::vec3(-1.0f),
                                       const glm::vec3& maxVelocity = glm::vec3(1.0f),
                                       const glm::vec4& startColor = glm::vec4(1.0f),
                                       const glm::vec4& endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
                                       int maxParticles = 1000);
    EntityBuilder& WithVideo(const std::string& videoPath, bool loop = true);
    EntityBuilder& WithVideoPlayback(bool playOnAwake, bool isPlaying, float volume = 1.0f, float speed = 1.0f,
                                     int maxDecodes = 1);
    EntityBuilder& WithPlayingVideo(const std::string& videoPath, bool loop = true, float volume = 1.0f,
                                    int maxDecodes = 1, float speed = 1.0f);
    EntityBuilder& WithUIVideo(const std::string& videoPath, const std::string& uiModelName,
                               const glm::vec4& color = glm::vec4(1.0f), bool loop = true, float volume = 1.0f,
                               int maxDecodes = 1, float speed = 1.0f);

    // Dedicated builder methods for formerly generic components
    EntityBuilder& WithInfo(const InfoComponent& info);
    EntityBuilder& WithPostProcess(const PostProcessComponent& postProcess);
    EntityBuilder& WithPostProcessEffect(const std::string& shaderName, int priority = 1, int x = 0, int y = 0,
                                         int w = 0, int h = 0);
    EntityBuilder& WithReflectionProbe(const ReflectionProbeComponent& probe);
    EntityBuilder& WithReflectionProbe(ReflectionProbeType type = ReflectionProbeType::Static, int resolution = 512,
                                       bool boxProjection = true);
    EntityBuilder& WithReflective(const ReflectiveComponent& reflective);
    EntityBuilder& WithReflective(float reflectivity = 1.0f, float fresnelPower = 5.0f, float fresnelBias = 0.04f);
    EntityBuilder& WithPlanarReflection(const PlanarReflectionComponent& planar);
    EntityBuilder& WithPlanarReflection(int resolution = 1024, const glm::vec3& normal = glm::vec3(0, 1, 0));
    EntityBuilder& WithLightProbe(const LightProbeComponent& probe);
    EntityBuilder& WithLightProbe(float radius = 5.0f, float intensity = 1.0f);
    EntityBuilder& WithRigidShape(const RigidShapeComponent& shape);
    EntityBuilder& WithRigidShape(ShapeType type, const glm::vec3& size = glm::vec3(1.0f), float radius = 1.0f,
                                  float height = 2.0f, float friction = 0.5f, float restitution = 0.0f);
    EntityBuilder& WithRigidBodyComponent(const RigidBodyComponent& body);
    EntityBuilder& WithDecal(const DecalComponent& decal);
    EntityBuilder& WithDecal(uint32_t albedoMap = 0, float opacity = 1.0f, float roughness = 0.5f,
                             float metallic = 0.0f, int lightingMode = 0, const glm::vec4& tintColor = glm::vec4(1.0f));
    EntityBuilder& WithAudioSource(const AudioSourceComponent& audioSource);
    EntityBuilder& WithAudioSource(const std::string& filePath, bool playOnAwake = true, bool loop = true,
                                   bool is3D = false, float volume = 100.0f, float pitch = 1.0f, float speed = 1.0f,
                                   float minDistance = 1.0f, float maxDistance = 100.0f);
    EntityBuilder& WithMeshRenderer(const MeshRendererComponent& meshRenderer);
    EntityBuilder& WithNavMesh(bool isDynamic = false, int terrainGridResolution = 64, float walkableNormalY = 0.3f);
    EntityBuilder& WithNavMesh(const NavMeshComponent& navMesh);
    EntityBuilder& WithNavigationGrid(int width, int height, float cellSize = 1.0f,
                                      const glm::vec3& origin = glm::vec3(0.0f), bool allowDiagonal = false);
    EntityBuilder& WithNavigationGrid(const NavigationGridComponent& grid);
    EntityBuilder& WithUIInteractive(bool interactable = true, std::function<void(entt::entity)> onClick = nullptr);
    EntityBuilder& WithUIInteractive(const UIInteractiveComponent& interactive);
    EntityBuilder& WithUIAnimation(bool enabled = true, bool animateColor = true, bool animateScale = false);
    EntityBuilder& WithUIAnimation(const UIAnimationComponent& anim);
    EntityBuilder& WithSkybox(const std::string& skyboxName, const std::string& shaderName, bool isPrimary = true);
    EntityBuilder& WithSkybox(const SkyboxRenderComponent& skybox);

    static entt::entity SpawnObject(Scene& scene, ResourceManager& res, const std::string& sceneName,
                                    const std::string& fragmentPath, const glm::vec3& pos,
                                    const glm::vec3& scale = glm::vec3(1.0f));

    static void ScatterObjects(Scene& scene, ResourceManager& res, const std::string& sceneName,
                               const std::string& fragmentPath, const PlacementRule& rule,
                               const std::vector<float>& heights, int width, int height, float terrainWidth,
                               float terrainLength, float terrainHeight, float waterLevel, float randomOffsetX,
                               float randomOffsetZ, int attempts = 200, const glm::vec3& scale = glm::vec3(1.0f));

    entt::entity Build();

private:
    Scene& m_Scene;
    ResourceManager& m_Resources;
    entt::entity m_Entity;
};
