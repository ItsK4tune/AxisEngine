#include <scene/logic/binary_scene_serializer.h>
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
#include <scene/type/scene_types.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <cmath>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
constexpr uint32_t MAX_BINARY_STRING_BYTES = 1024 * 1024;
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
void WriteEnum(std::ostream& os, T value)
{
    using Underlying = std::underlying_type_t<T>;
    WriteValue(os, static_cast<Underlying>(value));
}

template <typename T>
void ReadEnum(std::istream& is, T& value)
{
    using Underlying = std::underlying_type_t<T>;
    Underlying raw{};
    ReadValue(is, raw);
    value = static_cast<T>(raw);
}

void WriteBool(std::ostream& os, bool value)
{
    uint8_t raw = value ? 1 : 0;
    WriteValue(os, raw);
}

void ReadBool(std::istream& is, bool& value)
{
    uint8_t raw = 0;
    ReadValue(is, raw);
    value = raw != 0;
}

void WriteString(std::ostream& os, const std::string& value)
{
    uint32_t length = static_cast<uint32_t>((std::min)(value.size(), static_cast<size_t>(MAX_BINARY_STRING_BYTES)));
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

void WriteVec2(std::ostream& os, const glm::vec2& value)
{
    WriteValue(os, value.x);
    WriteValue(os, value.y);
}

void ReadVec2(std::istream& is, glm::vec2& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
}

void WriteVec3(std::ostream& os, const glm::vec3& value)
{
    WriteValue(os, value.x);
    WriteValue(os, value.y);
    WriteValue(os, value.z);
}

void ReadVec3(std::istream& is, glm::vec3& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
    ReadValue(is, value.z);
}

void WriteQuat(std::ostream& os, const glm::quat& value)
{
    WriteValue(os, value.x);
    WriteValue(os, value.y);
    WriteValue(os, value.z);
    WriteValue(os, value.w);
}

void ReadQuat(std::istream& is, glm::quat& value)
{
    ReadValue(is, value.x);
    ReadValue(is, value.y);
    ReadValue(is, value.z);
    ReadValue(is, value.w);
}

void WriteMat4(std::ostream& os, const glm::mat4& value)
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            WriteValue(os, value[col][row]);
        }
    }
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

void WriteAppConfigV3(std::ostream& os, const AppConfig& config)
{
    WriteString(os, config.title);
    WriteEnum(os, config.logLevel);
    WriteValue(os, config.numJobThreads);
    WriteValue(os, config.timeScale);
    WriteString(os, config.iconPath);
    WriteBool(os, config.headlessMode);

    WriteValue(os, config.width);
    WriteValue(os, config.height);
    WriteEnum(os, config.windowMode);
    WriteBool(os, config.vsync);
    WriteValue(os, config.monitorIndex);
    WriteValue(os, config.refreshRate);
    WriteValue(os, config.frameRateLimit);

    WriteEnum(os, config.graphicsBackend);
    WriteValue(os, config.msaaSamples);
    WriteValue(os, config.antialiasing);
    WriteValue(os, config.maxAnisotropy);
    WriteValue(os, config.renderScale);
    WriteBool(os, config.asyncResourceLoading);
    WriteBool(os, config.strictAssetLoading);

    WriteEnum(os, config.tonemappingMode);
    WriteBool(os, config.hdrEnabled);
    WriteBool(os, config.bloomEnabled);
    WriteValue(os, config.gamma);
    WriteValue(os, config.exposure);
    WriteValue(os, config.bloomIntensity);
    WriteValue(os, config.bloomThreshold);
    WriteValue(os, config.bloomRadius);
    WriteValue(os, config.skyboxIntensity);
    WriteValue(os, config.ambientIntensity);
    WriteValue(os, config.uiReferenceWidth);
    WriteValue(os, config.uiReferenceHeight);
    for (float channel : config.clearColor) WriteValue(os, channel);

    WriteBool(os, config.shadowsEnabled);
    WriteValue(os, config.shadowMode);
    WriteValue(os, config.shadowMapResolution);
    WriteValue(os, config.shadowProjectionSize);
    WriteBool(os, config.shadowFrustumCullingEnabled);
    WriteValue(os, config.shadowDistanceCulling);
    WriteValue(os, config.shadowBias);
    WriteValue(os, config.shadowSoftness);

    WriteEnum(os, config.physicsBackend);
    WriteEnum(os, config.physicsMode);
    for (float axis : config.gravity) WriteValue(os, axis);
    WriteValue(os, config.maxSubSteps);
    WriteValue(os, config.physicsTickRate);
    WriteBool(os, config.ccdEnabled);
    WriteValue(os, config.ccdThreshold);
    WriteValue(os, config.solverIterations);

    WriteValue(os, config.mouseSensitivityX);
    WriteValue(os, config.mouseSensitivityY);
    WriteBool(os, config.mouseInvertX);
    WriteBool(os, config.mouseInvertY);

    WriteEnum(os, config.audioBackend);
    WriteValue(os, config.masterVolume);
    WriteString(os, config.audioDevice);

    WriteBool(os, config.cullFaceEnabled);
    WriteBool(os, config.depthTestEnabled);
    WriteBool(os, config.frustumCullingEnabled);
    WriteBool(os, config.occlusionCullingEnabled);
    WriteBool(os, config.instanceBatchingEnabled);
    WriteBool(os, config.renderOrderEnabled);
    WriteValue(os, config.filterLayerMask);
    WriteValue(os, config.distanceCulling);

    WriteBool(os, config.debug.physicsDebug);
    WriteBool(os, config.debug.uiEnabled);
    WriteBool(os, config.debug.gizmos);
    WriteBool(os, config.debug.lightGizmos);
    WriteBool(os, config.debug.entityNames);
    WriteBool(os, config.debug.audioDebug);
    WriteBool(os, config.debug.particleDebug);
    WriteBool(os, config.debug.gridSnapEnabled);
    WriteBool(os, config.debug.gridIndicatorEnabled);
    WriteValue(os, config.debug.gridSnapTranslation);
    WriteValue(os, config.debug.gridSnapRotation);
    WriteValue(os, config.debug.gridSnapScale);

    WriteEnum(os, config.lightingMode);
}

void ReadAppConfigV3(std::istream& is, AppConfig& config)
{
    config.title = ReadString(is);
    ReadEnum(is, config.logLevel);
    ReadValue(is, config.numJobThreads);
    ReadValue(is, config.timeScale);
    config.iconPath = ReadString(is);
    ReadBool(is, config.headlessMode);

    ReadValue(is, config.width);
    ReadValue(is, config.height);
    ReadEnum(is, config.windowMode);
    ReadBool(is, config.vsync);
    ReadValue(is, config.monitorIndex);
    ReadValue(is, config.refreshRate);
    ReadValue(is, config.frameRateLimit);

    ReadEnum(is, config.graphicsBackend);
    ReadValue(is, config.msaaSamples);
    ReadValue(is, config.antialiasing);
    ReadValue(is, config.maxAnisotropy);
    ReadValue(is, config.renderScale);
    ReadBool(is, config.asyncResourceLoading);
    ReadBool(is, config.strictAssetLoading);

    ReadEnum(is, config.tonemappingMode);
    ReadBool(is, config.hdrEnabled);
    ReadBool(is, config.bloomEnabled);
    ReadValue(is, config.gamma);
    ReadValue(is, config.exposure);
    ReadValue(is, config.bloomIntensity);
    ReadValue(is, config.bloomThreshold);
    ReadValue(is, config.bloomRadius);
    ReadValue(is, config.skyboxIntensity);
    ReadValue(is, config.ambientIntensity);
    ReadValue(is, config.uiReferenceWidth);
    ReadValue(is, config.uiReferenceHeight);
    for (float& channel : config.clearColor) ReadValue(is, channel);

    ReadBool(is, config.shadowsEnabled);
    ReadValue(is, config.shadowMode);
    ReadValue(is, config.shadowMapResolution);
    ReadValue(is, config.shadowProjectionSize);
    ReadBool(is, config.shadowFrustumCullingEnabled);
    ReadValue(is, config.shadowDistanceCulling);
    ReadValue(is, config.shadowBias);
    ReadValue(is, config.shadowSoftness);

    ReadEnum(is, config.physicsBackend);
    ReadEnum(is, config.physicsMode);
    for (float& axis : config.gravity) ReadValue(is, axis);
    ReadValue(is, config.maxSubSteps);
    ReadValue(is, config.physicsTickRate);
    ReadBool(is, config.ccdEnabled);
    ReadValue(is, config.ccdThreshold);
    ReadValue(is, config.solverIterations);

    ReadValue(is, config.mouseSensitivityX);
    ReadValue(is, config.mouseSensitivityY);
    ReadBool(is, config.mouseInvertX);
    ReadBool(is, config.mouseInvertY);

    ReadEnum(is, config.audioBackend);
    ReadValue(is, config.masterVolume);
    config.audioDevice = ReadString(is);

    ReadBool(is, config.cullFaceEnabled);
    ReadBool(is, config.depthTestEnabled);
    ReadBool(is, config.frustumCullingEnabled);
    ReadBool(is, config.occlusionCullingEnabled);
    ReadBool(is, config.instanceBatchingEnabled);
    ReadBool(is, config.renderOrderEnabled);
    ReadValue(is, config.filterLayerMask);
    ReadValue(is, config.distanceCulling);

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
    config.audioDevice = ReadString(is);

    ReadValue(is, config.width);
    ReadValue(is, config.height);
    ReadEnum(is, config.windowMode);
    ReadLegacyBool(is, config.vsync);
    SkipLegacyBytes(is, 3);
    ReadValue(is, config.monitorIndex);
    ReadValue(is, config.refreshRate);
    ReadValue(is, config.frameRateLimit);

    ReadEnum(is, config.graphicsBackend);
    ReadValue(is, config.msaaSamples);
    ReadValue(is, config.antialiasing);
    ReadValue(is, config.maxAnisotropy);
    ReadValue(is, config.renderScale);
    ReadLegacyBool(is, config.asyncResourceLoading);
    ReadEnum(is, config.tonemappingMode);
    ReadLegacyBool(is, config.hdrEnabled);
    ReadLegacyBool(is, config.bloomEnabled);
    ReadValue(is, config.gamma);
    ReadValue(is, config.exposure);
    ReadValue(is, config.bloomIntensity);
    ReadValue(is, config.bloomThreshold);
    ReadValue(is, config.bloomRadius);
    ReadValue(is, config.skyboxIntensity);
    for (float& channel : config.clearColor) ReadValue(is, channel);

    ReadLegacyBool(is, config.shadowsEnabled);
    ReadValue(is, config.shadowMode);
    ReadValue(is, config.shadowMapResolution);
    ReadValue(is, config.shadowProjectionSize);
    ReadLegacyBool(is, config.shadowFrustumCullingEnabled);
    ReadValue(is, config.shadowDistanceCulling);
    ReadValue(is, config.shadowBias);
    ReadValue(is, config.shadowSoftness);

    ReadEnum(is, config.physicsBackend);
    ReadEnum(is, config.physicsMode);
    for (float& axis : config.gravity) ReadValue(is, axis);
    ReadValue(is, config.maxSubSteps);
    ReadValue(is, config.physicsTickRate);
    ReadLegacyBool(is, config.ccdEnabled);
    ReadValue(is, config.ccdThreshold);
    ReadValue(is, config.solverIterations);

    ReadValue(is, config.mouseSensitivityX);
    ReadValue(is, config.mouseSensitivityY);
    ReadLegacyBool(is, config.mouseInvertX);
    ReadLegacyBool(is, config.mouseInvertY);

    ReadEnum(is, config.audioBackend);
    ReadValue(is, config.masterVolume);

    ReadLegacyBool(is, config.cullFaceEnabled);
    ReadLegacyBool(is, config.depthTestEnabled);
    ReadLegacyBool(is, config.stencilTestEnabled);
    ReadLegacyBool(is, config.frustumCullingEnabled);
    ReadLegacyBool(is, config.occlusionCullingEnabled);
    ReadLegacyBool(is, config.renderOrderEnabled);
    ReadValue(is, config.filterLayerMask);
    ReadValue(is, config.distanceCulling);
}

template <typename T>
bool EnumInRange(T value, int minValue, int maxValue)
{
    int raw = static_cast<int>(value);
    return raw >= minValue && raw <= maxValue;
}

bool IsSaneAppConfig(const AppConfig& config)
{
    return EnumInRange(config.logLevel, 0, 4) && EnumInRange(config.windowMode, 0, 3) &&
           EnumInRange(config.graphicsBackend, 0, 2) && EnumInRange(config.tonemappingMode, 0, 2) &&
           EnumInRange(config.physicsBackend, 0, 1) && EnumInRange(config.physicsMode, 0, 2) &&
           EnumInRange(config.audioBackend, 0, 3) && EnumInRange(config.lightingMode, 0, 3) &&
           config.width > 0 && config.width <= 32768 && config.height > 0 && config.height <= 32768 &&
           config.msaaSamples >= 0 && config.msaaSamples <= 64 && config.maxSubSteps >= 0 &&
           config.maxSubSteps <= 1024 && config.physicsTickRate > 0.0f &&
           config.physicsTickRate <= 10000.0f && config.uiReferenceWidth > 0.0f &&
           config.uiReferenceHeight > 0.0f && std::isfinite(config.timeScale) &&
           std::isfinite(config.renderScale) && std::isfinite(config.physicsTickRate);
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
void WriteMaterialDescriptor(std::ostream& os, const MaterialDescriptor& desc)
{
    WriteValue(os, desc.pbr.roughness);
    WriteValue(os, desc.pbr.metallic);
    WriteValue(os, desc.pbr.ao);
    WriteValue(os, desc.opacity);
    WriteValue(os, desc.alphaCutoff);
    WriteVec3(os, desc.emission);
    WriteVec2(os, desc.uvScale);
    WriteVec2(os, desc.uvOffset);
    WriteString(os, desc.albedoPath);
    WriteString(os, desc.normalPath);
    WriteString(os, desc.metallicPath);
    WriteString(os, desc.roughnessPath);
    WriteString(os, desc.aoPath);
    WriteString(os, desc.emissivePath);
    WriteString(os, desc.specularPath);
    WriteEnum(os, desc.blendSrc);
    WriteEnum(os, desc.blendDst);
    WriteString(os, desc.type);
    for (float port : desc.ports.data) WriteValue(os, port);
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

void WriteCamera(std::ostream& os, const CameraComponent& camera)
{
    WriteMat4(os, camera.projectionMatrix);
    WriteMat4(os, camera.viewMatrix);
    WriteValue(os, camera.fov);
    WriteValue(os, camera.nearPlane);
    WriteValue(os, camera.farPlane);
    WriteValue(os, camera.aspectRatio);
    WriteValue(os, camera.screenWidth);
    WriteValue(os, camera.screenHeight);
    WriteBool(os, camera.isPrimary);
    WriteBool(os, camera.isOrthographic);
    WriteValue(os, camera.orthoSize);
    WriteValue(os, camera.cullingMask);
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

void WriteDirectionalLight(std::ostream& os, const DirectionalLightComponent& light)
{
    WriteVec3(os, light.direction);
    WriteVec3(os, light.color);
    WriteValue(os, light.intensity);
    WriteValue(os, light.ambient);
    WriteValue(os, light.diffuse);
    WriteValue(os, light.specular);
    WriteBool(os, light.active);
    WriteBool(os, light.isCastShadow);
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

void WritePointLight(std::ostream& os, const PointLightComponent& light)
{
    WriteVec3(os, light.color);
    WriteValue(os, light.radius);
    WriteValue(os, light.intensity);
    WriteValue(os, light.constant);
    WriteValue(os, light.linear);
    WriteValue(os, light.quadratic);
    WriteValue(os, light.ambient);
    WriteValue(os, light.diffuse);
    WriteValue(os, light.specular);
    WriteBool(os, light.active);
    WriteBool(os, light.isCastShadow);
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

void WriteSpotLight(std::ostream& os, const SpotLightComponent& light)
{
    WriteVec3(os, light.direction);
    WriteVec3(os, light.color);
    WriteValue(os, light.radius);
    WriteValue(os, light.intensity);
    WriteValue(os, light.constant);
    WriteValue(os, light.linear);
    WriteValue(os, light.quadratic);
    WriteValue(os, light.ambient);
    WriteValue(os, light.diffuse);
    WriteValue(os, light.specular);
    WriteValue(os, light.cutOff);
    WriteValue(os, light.outerCutOff);
    WriteBool(os, light.active);
    WriteBool(os, light.isCastShadow);
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
    std::ofstream os(filepath, std::ios::binary);
    if (!os.is_open())
        return false;

    WriteValue(os, scene::BINARY_MAGIC);
    WriteValue(os, VERSION);

    auto& mutableScene = const_cast<Scene&>(scene);
    auto view = mutableScene.View<InfoComponent>();
    std::vector<entt::entity> entities;
    for (auto entity : view) entities.push_back(entity);

    uint32_t entityCount = static_cast<uint32_t>(entities.size());
    WriteValue(os, entityCount);

    std::map<entt::entity, uint32_t> entityToIndex;
    uint32_t idx = 0;
    for (auto entity : entities) entityToIndex[entity] = idx++;

    for (auto entity : entities)
    {
        auto& info = mutableScene.GetComponent<InfoComponent>(entity);

        WriteString(os, info.name);
        WriteString(os, info.tag);
        WriteValue(os, info.layer);

        auto* pos = mutableScene.TryGetComponent<PositionComponent>(entity);
        WriteBool(os, pos != nullptr);
        if (pos)
            WriteVec3(os, pos->value);

        auto* rot = mutableScene.TryGetComponent<RotationComponent>(entity);
        WriteBool(os, rot != nullptr);
        if (rot)
            WriteQuat(os, rot->value);

        auto* scl = mutableScene.TryGetComponent<ScaleComponent>(entity);
        WriteBool(os, scl != nullptr);
        if (scl)
            WriteVec3(os, scl->value);

        auto* hier = mutableScene.TryGetComponent<HierarchyComponent>(entity);
        int32_t parentIdx = -1;
        if (hier && hier->parent != entt::null && entityToIndex.count(hier->parent))
            parentIdx = static_cast<int32_t>(entityToIndex[hier->parent]);
        WriteValue(os, parentIdx);

        auto* mesh = mutableScene.TryGetComponent<MeshRendererComponent>(entity);
        WriteBool(os, mesh != nullptr);
        if (mesh)
        {
            std::string modelName = mesh->model ? mesh->model->GetName() : "";
            WriteString(os, modelName);

            auto shaderPtr = mesh->shader.lock();
            std::string shaderName = shaderPtr ? shaderPtr->GetName() : mesh->shaderName;
            WriteString(os, shaderName);
            WriteBool(os, mesh->castShadow);
        }

        auto* mat = mutableScene.TryGetComponent<MaterialComponent>(entity);
        WriteBool(os, mat != nullptr);
        if (mat)
            WriteMaterialDescriptor(os, mat->desc);

        auto* cam = mutableScene.TryGetComponent<CameraComponent>(entity);
        WriteBool(os, cam != nullptr);
        if (cam)
            WriteCamera(os, *cam);

        auto* dl = mutableScene.TryGetComponent<DirectionalLightComponent>(entity);
        WriteBool(os, dl != nullptr);
        if (dl)
            WriteDirectionalLight(os, *dl);

        auto* pl = mutableScene.TryGetComponent<PointLightComponent>(entity);
        WriteBool(os, pl != nullptr);
        if (pl)
            WritePointLight(os, *pl);

        auto* spot = mutableScene.TryGetComponent<SpotLightComponent>(entity);
        WriteBool(os, spot != nullptr);
        if (spot)
            WriteSpotLight(os, *spot);

        auto* sky = mutableScene.TryGetComponent<SkyboxRenderComponent>(entity);
        WriteBool(os, sky != nullptr);
        if (sky)
        {
            WriteBool(os, sky->isPrimary);
            std::string skyboxName = sky->skybox ? sky->skybox->GetName() : "";
            WriteString(os, skyboxName);

            auto shaderPtr = sky->shader.lock();
            std::string shaderName = shaderPtr ? shaderPtr->GetName() : sky->shaderName;
            WriteString(os, shaderName);
        }
    }

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

        auto& info = scene.AddComponent<InfoComponent>(entity, name, tag);
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
                if (!material.desc.albedoPath.empty()) outResult.loadedTextures.push_back(material.desc.albedoPath);
                if (!material.desc.normalPath.empty()) outResult.loadedTextures.push_back(material.desc.normalPath);
                if (!material.desc.metallicPath.empty()) outResult.loadedTextures.push_back(material.desc.metallicPath);
                if (!material.desc.roughnessPath.empty()) outResult.loadedTextures.push_back(material.desc.roughnessPath);
                if (!material.desc.aoPath.empty()) outResult.loadedTextures.push_back(material.desc.aoPath);
                if (!material.desc.emissivePath.empty()) outResult.loadedTextures.push_back(material.desc.emissivePath);
                if (!material.desc.specularPath.empty()) outResult.loadedTextures.push_back(material.desc.specularPath);
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

    LOGGER_INFO("BinarySceneSerializer") << "Deserialized scene: " << path;
    return true;
}
