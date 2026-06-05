#include "test_framework.h"
#include "test_support.h"

#include <core/logic/config_manager.h>
#include <core/type/lighting_mode.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/binary_scene_serializer.h>

AXIS_TEST_CASE("BinarySceneSerializer round trips v3 config and material fields")
{
    axis_test_support::ResetServices();

    ConfigManager configManager;
    AppConfig config;
    config.strictAssetLoading = true;
    config.ambientIntensity = 0.42f;
    config.uiReferenceWidth = 1440.0f;
    config.uiReferenceHeight = 900.0f;
    config.solverIterations = 17;
    config.debug.physicsDebug = true;
    config.debug.gridSnapEnabled = true;
    config.debug.gridSnapTranslation = 0.25f;
    config.lightingMode = LightingMode::ReflectionProbes;
    configManager.Initialize(config);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    Scene source;
    auto entity = EntityManager::CreateEntity(source, "BinaryEntity", "serialize");
    source.registry.get<InfoComponent>(entity).layer = 7;

    auto& material = source.registry.emplace<AxisMaterialComponent>(entity);
    material.desc.albedoPath = "materials/albedo.png";
    material.desc.normalPath = "materials/normal.png";
    material.desc.opacity = 0.75f;
    material.desc.alphaCutoff = 0.2f;
    material.desc.emission = glm::vec3(0.1f, 0.2f, 0.3f);
    material.desc.uvScale = glm::vec2(2.0f, 3.0f);
    material.desc.blendSrc = BlendFactor::SrcAlpha;
    material.desc.blendDst = BlendFactor::One;
    material.desc.type = "CustomPBR";
    material.desc.ports.data[3] = 9.5f;

    auto& camera = source.registry.emplace<CameraComponent>(entity);
    camera.fov = 60.0f;
    camera.screenWidth = 1440;
    camera.screenHeight = 900;
    camera.isPrimary = true;

    auto& light = source.registry.emplace<DirectionalLightComponent>(entity);
    light.intensity = 3.0f;
    light.active = false;

    auto path = axis_test_support::TempPath("binary_scene_v3.axsb");
    AXIS_CHECK(BinarySceneSerializer::Save(path.string(), source));

    AppConfig resetConfig;
    configManager.Initialize(resetConfig);

    Scene loaded;
    AXIS_CHECK(BinarySceneSerializer::Load(path.string(), loaded));

    auto loadedConfig = configManager.GetConfig();
    AXIS_CHECK(loadedConfig.strictAssetLoading);
    AXIS_CHECK_NEAR(loadedConfig.ambientIntensity, 0.42f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.uiReferenceWidth, 1440.0f, 0.0001f);
    AXIS_CHECK_NEAR(loadedConfig.uiReferenceHeight, 900.0f, 0.0001f);
    AXIS_CHECK(loadedConfig.solverIterations == 17);
    AXIS_CHECK(loadedConfig.debug.physicsDebug);
    AXIS_CHECK(loadedConfig.debug.gridSnapEnabled);
    AXIS_CHECK_NEAR(loadedConfig.debug.gridSnapTranslation, 0.25f, 0.0001f);
    AXIS_CHECK(loadedConfig.lightingMode == LightingMode::ReflectionProbes);

    auto loadedEntity = EntityManager::FindByName(loaded, "BinaryEntity");
    AXIS_CHECK(loadedEntity != entt::null);
    AXIS_CHECK(loaded.registry.get<InfoComponent>(loadedEntity).tag == "serialize");
    AXIS_CHECK(loaded.registry.get<InfoComponent>(loadedEntity).layer == 7);

    auto& loadedMaterial = loaded.registry.get<AxisMaterialComponent>(loadedEntity);
    AXIS_CHECK(loadedMaterial.desc.albedoPath == "materials/albedo.png");
    AXIS_CHECK(loadedMaterial.desc.normalPath == "materials/normal.png");
    AXIS_CHECK(loadedMaterial.desc.type == "CustomPBR");
    AXIS_CHECK(loadedMaterial.desc.blendDst == BlendFactor::One);
    AXIS_CHECK_NEAR(loadedMaterial.desc.opacity, 0.75f, 0.0001f);
    AXIS_CHECK_NEAR(loadedMaterial.desc.uvScale.y, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(loadedMaterial.desc.ports.data[3], 9.5f, 0.0001f);

    auto& loadedCamera = loaded.registry.get<CameraComponent>(loadedEntity);
    AXIS_CHECK(loadedCamera.isPrimary);
    AXIS_CHECK(loadedCamera.screenWidth == 1440);
    AXIS_CHECK(loadedCamera.screenHeight == 900);
    AXIS_CHECK_NEAR(loadedCamera.fov, 60.0f, 0.0001f);

    auto& loadedLight = loaded.registry.get<DirectionalLightComponent>(loadedEntity);
    AXIS_CHECK(!loadedLight.active);
    AXIS_CHECK_NEAR(loadedLight.intensity, 3.0f, 0.0001f);

    axis_test_support::ResetServices();
}
