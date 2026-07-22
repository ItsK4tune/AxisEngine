#include <scene/logic/binary_scene_serializer.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/model.h>
#include <resource/unit/shader.h>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_load_finalizer.h>
#include <scene/logic/scene_serializer.h>
#include <scene/type/scene_types.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <map>
#include <memory>
#include <cmath>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
constexpr uint32_t MAX_BINARY_STRING_BYTES = 256 * 1024 * 1024;
constexpr std::streamsize LEGACY_AXIS_MATERIAL_DESCRIPTOR_V2_BYTES = 344;

template <typename T>
void WriteValue(std::ostream& os, const T& value)
{
    static_assert(std::is_arithmetic_v<T>, "Binary serializer only writes scalar arithmetic values here.");
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void ReadValue(std::istream& is, T& value)
{
    static_assert(std::is_arithmetic_v<T>, "Binary serializer only reads scalar arithmetic values here.");
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

template <typename T>
void ReadEnum(std::istream& is, T& value)
{
    using Underlying = std::underlying_type_t<T>;
    Underlying raw{};
    ReadValue(is, raw);
    value = static_cast<T>(raw);
}

void ReadBool(std::istream& is, bool& value)
{
    uint8_t raw = 0;
    ReadValue(is, raw);
    value = raw != 0;
}

void WriteString(std::ostream& os, const std::string& value)
{
    if (value.size() > MAX_BINARY_STRING_BYTES)
    {
        os.setstate(std::ios::failbit);
        return;
    }
    uint32_t length = static_cast<uint32_t>(value.size());
    WriteValue(os, length);
    if (length > 0)
        os.write(value.data(), length);
}

std::string ReadString(std::istream& is)
{
    uint32_t length = 0;
    ReadValue(is, length);
    if (length > MAX_BINARY_STRING_BYTES)
    {
        is.setstate(std::ios::failbit);
        return {};
    }

    std::string value(length, '\0');
    if (length > 0)
        is.read(value.data(), length);
    return value;
}

void ReadVec2(std::istream& is, glm::vec2& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
}

void ReadVec3(std::istream& is, glm::vec3& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
    ReadValue(is, value.z);
}

void ReadQuat(std::istream& is, glm::quat& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
    ReadValue(is, value.z);
    ReadValue(is, value.w);
}

void ReadMat4(std::istream& is, glm::mat4& value)
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            ReadValue(is, value[col][row]);
        }
    }
}

void ReadAppConfigV3(std::istream& is, AppConfig& config)
{
    config.title = ReadString(is);
    ReadEnum(is, config.logLevel);
    ReadValue(is, config.numJobThreads);
    ReadValue(is, config.timeScale);
    config.iconPath = ReadString(is);
    ReadBool(is, config.headlessMode);

    ReadValue(is, config.window.width);
    ReadValue(is, config.window.height);
    ReadEnum(is, config.window.windowMode);
    ReadBool(is, config.window.vsync);
    ReadValue(is, config.window.monitorIndex);
    ReadValue(is, config.window.refreshRate);
    ReadValue(is, config.window.frameRateLimit);

    ReadEnum(is, config.graphics.graphicsBackend);
    ReadValue(is, config.graphics.msaaSamples);
    ReadValue(is, config.graphics.antialiasing);
    ReadValue(is, config.graphics.maxAnisotropy);
    ReadValue(is, config.graphics.renderScale);
    ReadBool(is, config.graphics.asyncResourceLoading);
    ReadBool(is, config.graphics.strictAssetLoading);

    ReadEnum(is, config.render.tonemappingMode);
    ReadBool(is, config.render.hdrEnabled);
    ReadBool(is, config.render.bloomEnabled);
    ReadValue(is, config.render.gamma);
    ReadValue(is, config.render.exposure);
    ReadValue(is, config.render.bloomIntensity);
    ReadValue(is, config.render.bloomThreshold);
    ReadValue(is, config.render.bloomRadius);
    ReadValue(is, config.render.skyboxIntensity);
    ReadValue(is, config.render.ambientIntensity);
    ReadValue(is, config.render.uiReferenceWidth);
    ReadValue(is, config.render.uiReferenceHeight);
    for (float& channel : config.render.clearColor) ReadValue(is, channel);

    ReadBool(is, config.shadow.shadowsEnabled);
    ReadValue(is, config.shadow.shadowMode);
    ReadValue(is, config.shadow.shadowMapResolution);
    ReadValue(is, config.shadow.shadowProjectionSize);
    ReadBool(is, config.shadow.shadowFrustumCullingEnabled);
    ReadValue(is, config.shadow.shadowDistanceCulling);
    ReadValue(is, config.shadow.shadowBias);
    ReadValue(is, config.shadow.shadowSoftness);

    ReadEnum(is, config.physics.physicsBackend);
    ReadEnum(is, config.physics.physicsMode);
    for (float& axis : config.physics.gravity) ReadValue(is, axis);
    ReadValue(is, config.physics.maxSubSteps);
    ReadValue(is, config.physics.physicsTickRate);
    ReadBool(is, config.physics.ccdEnabled);
    ReadValue(is, config.physics.ccdThreshold);
    ReadValue(is, config.physics.solverIterations);

    ReadValue(is, config.input.mouseSensitivityX);
    ReadValue(is, config.input.mouseSensitivityY);
    ReadBool(is, config.input.mouseInvertX);
    ReadBool(is, config.input.mouseInvertY);

    ReadEnum(is, config.audio.audioBackend);
    ReadValue(is, config.audio.masterVolume);
    config.audio.audioDevice = ReadString(is);

    ReadBool(is, config.culling.cullFaceEnabled);
    ReadBool(is, config.culling.depthTestEnabled);
    ReadBool(is, config.culling.frustumCullingEnabled);
    ReadBool(is, config.culling.occlusionCullingEnabled);
    ReadBool(is, config.culling.instanceBatchingEnabled);
    ReadBool(is, config.culling.renderOrderEnabled);
    ReadValue(is, config.culling.filterLayerMask);
    ReadValue(is, config.culling.distanceCulling);

    ReadBool(is, config.debug.physicsDebug);
    ReadBool(is, config.debug.uiEnabled);
    ReadBool(is, config.debug.gizmos);
    ReadBool(is, config.debug.lightGizmos);
    ReadBool(is, config.debug.entityNames);
    ReadBool(is, config.debug.audioDebug);
    ReadBool(is, config.debug.particleDebug);
    ReadBool(is, config.debug.gridSnapEnabled);
    ReadBool(is, config.debug.gridIndicatorEnabled);
    ReadValue(is, config.debug.gridSnapTranslation);
    ReadValue(is, config.debug.gridSnapRotation);
    ReadValue(is, config.debug.gridSnapScale);

    ReadEnum(is, config.lightingMode);
}
void SkipLegacyBytes(std::istream& is, std::streamsize count);
void ReadLegacyBool(std::istream& is, bool& value);

void ReadLegacyAppConfigV2(std::istream& is, AppConfig& config)
{
    config.title = ReadString(is);
    ReadEnum(is, config.logLevel);
    ReadValue(is, config.numJobThreads);
    ReadValue(is, config.timeScale);
    config.iconPath = ReadString(is);
    config.audio.audioDevice = ReadString(is);

    ReadValue(is, config.window.width);
    ReadValue(is, config.window.height);
    ReadEnum(is, config.window.windowMode);
    ReadLegacyBool(is, config.window.vsync);
    SkipLegacyBytes(is, 3);
    ReadValue(is, config.window.monitorIndex);
    ReadValue(is, config.window.refreshRate);
    ReadValue(is, config.window.frameRateLimit);

    ReadEnum(is, config.graphics.graphicsBackend);
    ReadValue(is, config.graphics.msaaSamples);
    ReadValue(is, config.graphics.antialiasing);
    ReadValue(is, config.graphics.maxAnisotropy);
    ReadValue(is, config.graphics.renderScale);
    ReadLegacyBool(is, config.graphics.asyncResourceLoading);
    ReadEnum(is, config.render.tonemappingMode);
    ReadLegacyBool(is, config.render.hdrEnabled);
    ReadLegacyBool(is, config.render.bloomEnabled);
    ReadValue(is, config.render.gamma);
    ReadValue(is, config.render.exposure);
    ReadValue(is, config.render.bloomIntensity);
    ReadValue(is, config.render.bloomThreshold);
    ReadValue(is, config.render.bloomRadius);
    ReadValue(is, config.render.skyboxIntensity);
    for (float& channel : config.render.clearColor) ReadValue(is, channel);

    ReadLegacyBool(is, config.shadow.shadowsEnabled);
    ReadValue(is, config.shadow.shadowMode);
    ReadValue(is, config.shadow.shadowMapResolution);
    ReadValue(is, config.shadow.shadowProjectionSize);
    ReadLegacyBool(is, config.shadow.shadowFrustumCullingEnabled);
    ReadValue(is, config.shadow.shadowDistanceCulling);
    ReadValue(is, config.shadow.shadowBias);
    ReadValue(is, config.shadow.shadowSoftness);

    ReadEnum(is, config.physics.physicsBackend);
    ReadEnum(is, config.physics.physicsMode);
    for (float& axis : config.physics.gravity) ReadValue(is, axis);
    ReadValue(is, config.physics.maxSubSteps);
    ReadValue(is, config.physics.physicsTickRate);
    ReadLegacyBool(is, config.physics.ccdEnabled);
    ReadValue(is, config.physics.ccdThreshold);
    ReadValue(is, config.physics.solverIterations);

    ReadValue(is, config.input.mouseSensitivityX);
    ReadValue(is, config.input.mouseSensitivityY);
    ReadLegacyBool(is, config.input.mouseInvertX);
    ReadLegacyBool(is, config.input.mouseInvertY);

    ReadEnum(is, config.audio.audioBackend);
    ReadValue(is, config.audio.masterVolume);

    ReadLegacyBool(is, config.culling.cullFaceEnabled);
    ReadLegacyBool(is, config.culling.depthTestEnabled);
    ReadLegacyBool(is, config.culling.stencilTestEnabled);
    ReadLegacyBool(is, config.culling.frustumCullingEnabled);
    ReadLegacyBool(is, config.culling.occlusionCullingEnabled);
    ReadLegacyBool(is, config.culling.renderOrderEnabled);
    ReadValue(is, config.culling.filterLayerMask);
    ReadValue(is, config.culling.distanceCulling);
}

template <typename T>
bool EnumInRange(T value, int minValue, int maxValue)
{
    int raw = static_cast<int>(value);
    return raw >= minValue && raw <= maxValue;
}

bool IsSaneAppConfig(const AppConfig& config)
{
    return EnumInRange(config.logLevel, 0, 4) && EnumInRange(config.window.windowMode, 0, 3) &&
           EnumInRange(config.graphics.graphicsBackend, 0, 2) && EnumInRange(config.render.tonemappingMode, 0, 2) &&
           EnumInRange(config.physics.physicsBackend, 0, 1) && EnumInRange(config.physics.physicsMode, 0, 2) &&
           EnumInRange(config.audio.audioBackend, 0, 3) && EnumInRange(config.lightingMode, 0, 3) &&
           config.window.width > 0 && config.window.width <= 32768 && config.window.height > 0 &&
           config.window.height <= 32768 && config.graphics.msaaSamples >= 0 && config.graphics.msaaSamples <= 64 &&
           config.physics.maxSubSteps >= 0 && config.physics.maxSubSteps <= 1024 &&
           config.physics.physicsTickRate > 0.0f && config.physics.physicsTickRate <= 10000.0f &&
           config.render.uiReferenceWidth > 0.0f && config.render.uiReferenceHeight > 0.0f &&
           std::isfinite(config.timeScale) && std::isfinite(config.graphics.renderScale) &&
           std::isfinite(config.physics.physicsTickRate);
}

bool TryReadLegacyEmbeddedConfig(std::istream& is, uint32_t version, AppConfig& config)
{
    std::streampos configStart = is.tellg();
    AppConfig candidate = config;

    if (version >= 3)
        ReadAppConfigV3(is, candidate);
    else
        ReadLegacyAppConfigV2(is, candidate);

    if (!is.good() || !IsSaneAppConfig(candidate))
    {
        is.clear();
        is.seekg(configStart);
        return false;
    }

    config = candidate;
    return true;
}
void ReadMaterialDescriptor(std::istream& is, MaterialDescriptor& desc)
{
    ReadValue(is, desc.pbr.roughness);
    ReadValue(is, desc.pbr.metallic);
    ReadValue(is, desc.pbr.ao);
    ReadValue(is, desc.opacity);
    ReadValue(is, desc.alphaCutoff);
    ReadVec3(is, desc.emission);
    ReadVec2(is, desc.uvScale);
    ReadVec2(is, desc.uvOffset);
    desc.albedoPath = ReadString(is);
    desc.normalPath = ReadString(is);
    desc.metallicPath = ReadString(is);
    desc.roughnessPath = ReadString(is);
    desc.aoPath = ReadString(is);
    desc.emissivePath = ReadString(is);
    desc.specularPath = ReadString(is);
    ReadEnum(is, desc.blendSrc);
    ReadEnum(is, desc.blendDst);
    desc.type = ReadString(is);
    for (float& port : desc.ports.data) ReadValue(is, port);
}

void ReadCamera(std::istream& is, CameraComponent& camera)
{
    ReadMat4(is, camera.projectionMatrix);
    ReadMat4(is, camera.viewMatrix);
    ReadValue(is, camera.fov);
    ReadValue(is, camera.nearPlane);
    ReadValue(is, camera.farPlane);
    ReadValue(is, camera.aspectRatio);
    ReadValue(is, camera.screenWidth);
    ReadValue(is, camera.screenHeight);
    ReadBool(is, camera.isPrimary);
    ReadBool(is, camera.isOrthographic);
    ReadValue(is, camera.orthoSize);
    ReadValue(is, camera.cullingMask);
}

void ReadDirectionalLight(std::istream& is, DirectionalLightComponent& light)
{
    ReadVec3(is, light.direction);
    ReadVec3(is, light.color);
    ReadValue(is, light.intensity);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadBool(is, light.active);
    ReadBool(is, light.isCastShadow);
}

void ReadPointLight(std::istream& is, PointLightComponent& light)
{
    ReadVec3(is, light.color);
    ReadValue(is, light.radius);
    ReadValue(is, light.intensity);
    ReadValue(is, light.constant);
    ReadValue(is, light.linear);
    ReadValue(is, light.quadratic);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadBool(is, light.active);
    ReadBool(is, light.isCastShadow);
}

void ReadSpotLight(std::istream& is, SpotLightComponent& light)
{
    ReadVec3(is, light.direction);
    ReadVec3(is, light.color);
    ReadValue(is, light.radius);
    ReadValue(is, light.intensity);
    ReadValue(is, light.constant);
    ReadValue(is, light.linear);
    ReadValue(is, light.quadratic);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadValue(is, light.cutOff);
    ReadValue(is, light.outerCutOff);
    ReadBool(is, light.active);
    ReadBool(is, light.isCastShadow);
}

void SkipLegacyBytes(std::istream& is, std::streamsize count)
{
    if (count > 0)
        is.ignore(count);
}

void ReadLegacyBool(std::istream& is, bool& value)
{
    uint8_t raw = 0;
    ReadValue(is, raw);
    value = raw != 0;
}

void ReadLegacyCameraV2(std::istream& is, CameraComponent& camera)
{
    ReadMat4(is, camera.projectionMatrix);
    ReadMat4(is, camera.viewMatrix);
    ReadValue(is, camera.fov);
    ReadValue(is, camera.nearPlane);
    ReadValue(is, camera.farPlane);
    ReadValue(is, camera.aspectRatio);
    ReadValue(is, camera.screenWidth);
    ReadValue(is, camera.screenHeight);
    ReadLegacyBool(is, camera.isPrimary);
    ReadLegacyBool(is, camera.isOrthographic);
    SkipLegacyBytes(is, 2);
    ReadValue(is, camera.orthoSize);
    ReadValue(is, camera.cullingMask);
}

void ReadLegacyDirectionalLightV2(std::istream& is, DirectionalLightComponent& light)
{
    ReadVec3(is, light.direction);
    ReadVec3(is, light.color);
    ReadValue(is, light.intensity);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadLegacyBool(is, light.active);
    ReadLegacyBool(is, light.isCastShadow);
    SkipLegacyBytes(is, 2);
}

void ReadLegacyPointLightV2(std::istream& is, PointLightComponent& light)
{
    ReadVec3(is, light.color);
    ReadValue(is, light.radius);
    ReadValue(is, light.intensity);
    ReadValue(is, light.constant);
    ReadValue(is, light.linear);
    ReadValue(is, light.quadratic);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadLegacyBool(is, light.active);
    ReadLegacyBool(is, light.isCastShadow);
    SkipLegacyBytes(is, 2);
}

void ReadLegacySpotLightV2(std::istream& is, SpotLightComponent& light)
{
    ReadVec3(is, light.direction);
    ReadVec3(is, light.color);
    ReadValue(is, light.radius);
    ReadValue(is, light.intensity);
    ReadValue(is, light.constant);
    ReadValue(is, light.linear);
    ReadValue(is, light.quadratic);
    ReadValue(is, light.ambient);
    ReadValue(is, light.diffuse);
    ReadValue(is, light.specular);
    ReadValue(is, light.cutOff);
    ReadValue(is, light.outerCutOff);
    ReadLegacyBool(is, light.active);
    ReadLegacyBool(is, light.isCastShadow);
    SkipLegacyBytes(is, 2);
}
}  // namespace

bool BinarySceneSerializer::Serialize(const std::string& filepath, const Scene& scene)
{
    auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
    std::unique_ptr<ResourceManager> fallbackResources;
    if (!resources)
    {
        fallbackResources = std::make_unique<ResourceManager>();
        fallbackResources->InitializeHeadless();
        resources = fallbackResources.get();
    }

    SceneSerializer sceneSerializer(*resources, ServiceLocator::Instance().Resolve<IPhysicsWorld>(),
                                    ServiceLocator::Instance().Resolve<AudioService>());
    const std::string payload = sceneSerializer.SerializeToString(scene);
    if (payload.empty())
        return false;

    std::ofstream os(filepath, std::ios::binary);
    if (!os.is_open())
        return false;

    WriteValue(os, scene::BINARY_MAGIC);
    WriteValue(os, VERSION);
    WriteString(os, payload);

    if (!os.good())
        return false;

    LOGGER_INFO("BinarySceneSerializer") << "Serialized scene to: " << filepath;
    return true;
}

bool BinarySceneSerializer::Deserialize(const std::string& filepath, Scene& scene)
{
    SceneLoadResult dummy;
    return Deserialize(filepath, scene, dummy);
}

bool BinarySceneSerializer::Deserialize(const std::string& path, Scene& scene, SceneLoadResult& outResult)
{
    std::ifstream is(path, std::ios::binary);
    if (!is.is_open())
        return false;

    uint32_t magic = 0;
    uint32_t version = 0;
    ReadValue(is, magic);
    ReadValue(is, version);

    if (magic != scene::BINARY_MAGIC)
        return false;

    if (version == 0 || version > VERSION)
    {
        LOGGER_ERROR("BinarySceneSerializer") << "Unsupported binary scene version: " << version;
        return false;
    }

    if (version == VERSION)
    {
        const std::string payload = ReadString(is);
        if (!is.good() || payload.empty())
            return false;

        auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
        std::unique_ptr<ResourceManager> fallbackResources;
        if (!resources)
        {
            fallbackResources = std::make_unique<ResourceManager>();
            fallbackResources->InitializeHeadless();
            resources = fallbackResources.get();
        }

        SceneSerializer sceneSerializer(*resources, ServiceLocator::Instance().Resolve<IPhysicsWorld>(),
                                        ServiceLocator::Instance().Resolve<AudioService>());
        const std::string sourceName = std::filesystem::path(path).stem().string();
        const bool loaded = sceneSerializer.DeserializeFromString(payload, sourceName, scene, outResult);
        if (loaded)
            LOGGER_INFO("BinarySceneSerializer") << "Deserialized v5 scene: " << path;
        return loaded;
    }

    uint32_t entityCount = 0;
    ReadValue(is, entityCount);

    if (version < 4)
    {
        auto* configMgr = ServiceLocator::Instance().Resolve<ConfigManager>();
        if (version >= 2)
        {
            AppConfig config = configMgr ? configMgr->GetConfig() : AppConfig{};
            if (TryReadLegacyEmbeddedConfig(is, version, config) && configMgr)
            {
                configMgr->UpdateConfig(config);
            }
        }
    }

    if (!is.good())
        return false;

    auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
    std::vector<entt::entity> entities;
    std::vector<int32_t> parents;
    entities.reserve(entityCount);
    parents.reserve(entityCount);

    for (uint32_t i = 0; i < entityCount; ++i)
    {
        std::string name = ReadString(is);
        std::string tag = ReadString(is);
        uint32_t layer = 1;
        ReadValue(is, layer);

        entt::entity entity = scene.GetRegistry().create();
        entities.push_back(entity);

        auto& info = scene.AddComponent<InfoComponent>(entity, name, CanonicalizeEntityTag(std::move(tag)));
        info.layer = layer;

        bool hasP = false;
        bool hasR = false;
        bool hasS = false;
        ReadBool(is, hasP);
        if (hasP)
        {
            glm::vec3 value;
            if (version >= 3)
                ReadVec3(is, value);
            else
                is.read(reinterpret_cast<char*>(&value), sizeof(value));
            scene.AddComponent<PositionComponent>(entity, value, value);
        }

        ReadBool(is, hasR);
        if (hasR)
        {
            glm::quat value;
            if (version >= 3)
                ReadQuat(is, value);
            else
                is.read(reinterpret_cast<char*>(&value), sizeof(value));
            scene.AddComponent<RotationComponent>(entity, value, value);
        }

        ReadBool(is, hasS);
        if (hasS)
        {
            glm::vec3 value;
            if (version >= 3)
                ReadVec3(is, value);
            else
                is.read(reinterpret_cast<char*>(&value), sizeof(value));
            scene.AddComponent<ScaleComponent>(entity, value, value);
        }

        int32_t parentIdx = -1;
        ReadValue(is, parentIdx);
        parents.push_back(parentIdx);

        bool hasMesh = false;
        ReadBool(is, hasMesh);
        if (hasMesh)
        {
            std::string modelName = ReadString(is);
            std::string shaderName = ReadString(is);
            bool castShadow = true;
            ReadBool(is, castShadow);

            auto& mesh = scene.AddComponent<MeshRendererComponent>(entity);
            mesh.modelName = modelName;
            if (resources)
            {
                mesh.model = resources->GetModel(modelName);
                mesh.shader = resources->GetShader(shaderName);
            }
            mesh.shaderName = shaderName;
            mesh.castShadow = castShadow;

            if (!modelName.empty())
                outResult.loadedModels.push_back(modelName);
            if (!shaderName.empty())
                outResult.loadedShaders.push_back(shaderName);
        }

        bool hasMat = false;
        ReadBool(is, hasMat);
        if (hasMat)
        {
            auto& material = scene.AddComponent<MaterialComponent>(entity);
            if (version >= 3)
            {
                ReadMaterialDescriptor(is, material.desc);
                if (!material.desc.albedoPath.empty())
                    outResult.loadedTextures.push_back(material.desc.albedoPath);
                if (!material.desc.normalPath.empty())
                    outResult.loadedTextures.push_back(material.desc.normalPath);
                if (!material.desc.metallicPath.empty())
                    outResult.loadedTextures.push_back(material.desc.metallicPath);
                if (!material.desc.roughnessPath.empty())
                    outResult.loadedTextures.push_back(material.desc.roughnessPath);
                if (!material.desc.aoPath.empty())
                    outResult.loadedTextures.push_back(material.desc.aoPath);
                if (!material.desc.emissivePath.empty())
                    outResult.loadedTextures.push_back(material.desc.emissivePath);
                if (!material.desc.specularPath.empty())
                    outResult.loadedTextures.push_back(material.desc.specularPath);
            }
            else
            {
                SkipLegacyBytes(is, LEGACY_AXIS_MATERIAL_DESCRIPTOR_V2_BYTES);
            }
            material.gpu.dirty = true;
            material.gpu.batchKeyDirty = true;
        }

        bool hasCam = false;
        ReadBool(is, hasCam);
        if (hasCam)
        {
            auto& camera = scene.AddComponent<CameraComponent>(entity);
            if (version >= 3)
                ReadCamera(is, camera);
            else
                ReadLegacyCameraV2(is, camera);
        }

        bool hasDL = false;
        ReadBool(is, hasDL);
        if (hasDL)
        {
            auto& light = scene.AddComponent<DirectionalLightComponent>(entity);
            if (version >= 3)
                ReadDirectionalLight(is, light);
            else
                ReadLegacyDirectionalLightV2(is, light);
        }

        bool hasPL = false;
        ReadBool(is, hasPL);
        if (hasPL)
        {
            auto& light = scene.AddComponent<PointLightComponent>(entity);
            if (version >= 3)
                ReadPointLight(is, light);
            else
                ReadLegacyPointLightV2(is, light);
        }

        bool hasSL = false;
        ReadBool(is, hasSL);
        if (hasSL)
        {
            auto& light = scene.AddComponent<SpotLightComponent>(entity);
            if (version >= 3)
                ReadSpotLight(is, light);
            else
                ReadLegacySpotLightV2(is, light);
        }

        bool hasSky = false;
        ReadBool(is, hasSky);
        if (hasSky)
        {
            auto& sky = scene.AddComponent<SkyboxRenderComponent>(entity);
            ReadBool(is, sky.isPrimary);
            std::string skyboxName = ReadString(is);
            sky.skyboxName = skyboxName;
            std::string shaderName = ReadString(is);
            if (resources)
            {
                sky.skybox = resources->GetSkybox(skyboxName);
                sky.shader = resources->GetShader(shaderName);
            }
            sky.shaderName = shaderName;
            if (sky.isPrimary)
                scene.SetActiveSkybox(entity);

            if (!skyboxName.empty())
                outResult.loadedSkyboxes.push_back(skyboxName);
            if (!shaderName.empty())
                outResult.loadedShaders.push_back(shaderName);
        }

        scene.AddComponent<HierarchyComponent>(entity);
        scene.AddComponent<WorldTransformComponent>(entity);

        if (!is.good())
            return false;
    }

    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (parents[i] != -1 && static_cast<size_t>(parents[i]) < entities.size())
            scene.SetParent(entities[i], entities[parents[i]]);
    }

    outResult.entities = entities;

    if (!SceneHandlers::SceneLoadFinalizer::Finalize(scene, outResult,
                                                     ServiceLocator::Instance().Resolve<IPhysicsWorld>()))
        return false;

    LOGGER_INFO("BinarySceneSerializer") << "Deserialized scene: " << path;
    return true;
}
