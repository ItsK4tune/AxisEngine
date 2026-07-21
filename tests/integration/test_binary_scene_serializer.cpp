#include "test_framework.h"
#include "test_support.h"

#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <navigation/unit/pathfollower_component.h>
#include <scene/logic/binary_scene_serializer.h>

AXIS_TEST_CASE("BinarySceneSerializer v5 round trips the complete scene schema")
{
    axis_test_support::ResetServices();

    ConfigManager configManager;
    configManager.Initialize(AppConfig{});
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    Scene source;
    auto entity = source.CreateEntity("BinaryEntity", "serialize");
    source.GetComponent<InfoComponent>(entity).layer = 7;
    source.GetComponent<InfoComponent>(entity).isActive = false;
    source.GetComponent<InfoComponent>(entity).renderOrder = 12;

    auto& material = source.AddComponent<MaterialComponent>(entity);
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

    auto& camera = source.AddComponent<CameraComponent>(entity);
    camera.fov = 60.0f;
    camera.screenWidth = 1440;
    camera.screenHeight = 900;
    camera.isPrimary = true;

    auto& light = source.AddComponent<DirectionalLightComponent>(entity);
    light.intensity = 3.0f;
    light.active = false;
    light.isCastShadow = false;

    auto& controller = source.AddComponent<CharacterControllerComponent>(entity);
    controller.radius = 0.8f;
    controller.height = 1.4f;
    controller.stepHeight = 0.42f;

    auto& video = source.AddComponent<VideoPlayerComponent>(entity);
    video.filePath = "asset://video/intro.mp4";
    video.isLooping = true;
    video.maxDecodes = 3;

    auto& decal = source.AddComponent<DecalComponent>(entity);
    decal.albedoTexture = "asset://textures/decal.png";
    decal.targetTags = {"enemy", "terrain"};
    decal.lightingMode = 2;

    auto& postProcess = source.AddComponent<PostProcessComponent>(entity);
    PostProcessComponent::Effect pulseEffect;
    pulseEffect.shaderName = "pulse_shader";
    pulseEffect.priority = 4;
    pulseEffect.w = 320;
    pulseEffect.h = 180;
    pulseEffect.affectUI = false;
    pulseEffect.inputs = PostProcessInput::Depth | PostProcessInput::Normal | PostProcessInput::AudioPulses;
    postProcess.effects.push_back(pulseEffect);

    auto& follower = source.AddComponent<PathFollowerComponent>(entity);
    follower.rotationSpeed = 7.0f;
    follower.lockMoveY = true;
    follower.pathfindingOptions.criteria = PathfindingCriteria::HighGround;

    auto& uiAnimation = source.AddComponent<UIAnimationComponent>(entity);
    uiAnimation.animateScale = true;
    uiAnimation.hoverScale = 1.2f;

    auto child = source.CreateEntity("BinaryChild");
    source.SetParent(child, entity);

    auto path = axis_test_support::TempPath("binary_scene_v5.axsb");
    BinarySceneSerializer serializer;
    AXIS_CHECK(serializer.Serialize(path.string(), source));

    Scene loaded;
    SceneLoadResult loadResult;
    AXIS_CHECK(serializer.Deserialize(path.string(), loaded, loadResult));
    AXIS_CHECK(!loadResult.validation.HasErrors());

    auto loadedEntity = loaded.FindByName("BinaryEntity");
    AXIS_CHECK(loadedEntity != entt::null);
    AXIS_CHECK(loaded.GetComponent<InfoComponent>(loadedEntity).tag == "serialize");
    AXIS_CHECK(loaded.GetComponent<InfoComponent>(loadedEntity).layer == 7);
    AXIS_CHECK(!loaded.GetComponent<InfoComponent>(loadedEntity).isActive);
    AXIS_CHECK(loaded.GetComponent<InfoComponent>(loadedEntity).renderOrder == 12);

    auto& loadedMaterial = loaded.GetComponent<MaterialComponent>(loadedEntity);
    AXIS_CHECK(loadedMaterial.desc.albedoPath == "materials/albedo.png");
    AXIS_CHECK(loadedMaterial.desc.normalPath == "materials/normal.png");
    AXIS_CHECK(loadedMaterial.desc.type == "CustomPBR");
    AXIS_CHECK(loadedMaterial.desc.blendDst == BlendFactor::One);
    AXIS_CHECK_NEAR(loadedMaterial.desc.opacity, 0.75f, 0.0001f);
    AXIS_CHECK_NEAR(loadedMaterial.desc.uvScale.y, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(loadedMaterial.desc.ports.data[3], 9.5f, 0.0001f);

    auto& loadedCamera = loaded.GetComponent<CameraComponent>(loadedEntity);
    AXIS_CHECK(loadedCamera.isPrimary);
    AXIS_CHECK(loadedCamera.screenWidth == 1440);
    AXIS_CHECK(loadedCamera.screenHeight == 900);
    AXIS_CHECK_NEAR(loadedCamera.fov, 60.0f, 0.0001f);

    auto& loadedLight = loaded.GetComponent<DirectionalLightComponent>(loadedEntity);
    AXIS_CHECK(!loadedLight.active);
    AXIS_CHECK_NEAR(loadedLight.intensity, 3.0f, 0.0001f);

    const auto& loadedController = loaded.GetComponent<CharacterControllerComponent>(loadedEntity);
    AXIS_CHECK_NEAR(loadedController.radius, 0.8f, 0.0001f);
    AXIS_CHECK_NEAR(loadedController.height, 1.4f, 0.0001f);
    AXIS_CHECK_NEAR(loadedController.stepHeight, 0.42f, 0.0001f);

    const auto& loadedVideo = loaded.GetComponent<VideoPlayerComponent>(loadedEntity);
    AXIS_CHECK(loadedVideo.filePath == "asset://video/intro.mp4");
    AXIS_CHECK(loadedVideo.isLooping);
    AXIS_CHECK(loadedVideo.maxDecodes == 3);

    const auto& loadedDecal = loaded.GetComponent<DecalComponent>(loadedEntity);
    AXIS_CHECK(loadedDecal.targetTags.size() == 2);
    AXIS_CHECK(loadedDecal.lightingMode == 2);

    const auto& loadedPostProcess = loaded.GetComponent<PostProcessComponent>(loadedEntity);
    AXIS_CHECK(loadedPostProcess.effects.size() == 1);
    AXIS_CHECK(HasPostProcessInput(loadedPostProcess.effects[0].inputs, PostProcessInput::AudioPulses));

    const auto& loadedFollower = loaded.GetComponent<PathFollowerComponent>(loadedEntity);
    AXIS_CHECK_NEAR(loadedFollower.rotationSpeed, 7.0f, 0.0001f);
    AXIS_CHECK(loadedFollower.lockMoveY);
    AXIS_CHECK(loadedFollower.pathfindingOptions.criteria == PathfindingCriteria::HighGround);

    const auto& loadedAnimation = loaded.GetComponent<UIAnimationComponent>(loadedEntity);
    AXIS_CHECK(loadedAnimation.animateScale);
    AXIS_CHECK_NEAR(loadedAnimation.hoverScale, 1.2f, 0.0001f);

    const auto loadedChild = loaded.FindByName("BinaryChild");
    AXIS_CHECK(loadedChild != entt::null);
    AXIS_CHECK(loaded.GetComponent<HierarchyComponent>(loadedChild).parent == loadedEntity);

    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("BinarySceneSerializer does not treat configless v3 scenes as config")
{
    axis_test_support::ResetServices();

    ConfigManager configManager;
    AppConfig config;
    config.debug.lightGizmos = false;
    configManager.Initialize(config);
    ServiceLocator::Instance().Register<ConfigManager>(&configManager);

    auto path = axis_test_support::TempPath("configless_v3_scene.axsb");
    {
        std::ofstream os(path, std::ios::binary);
        auto writeU32 = [&](uint32_t value) { os.write(reinterpret_cast<const char*>(&value), sizeof(value)); };
        auto writeI32 = [&](int32_t value) { os.write(reinterpret_cast<const char*>(&value), sizeof(value)); };
        auto writeBool = [&](bool value) {
            uint8_t raw = value ? 1 : 0;
            os.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
        };
        auto writeString = [&](const std::string& value) {
            writeU32(static_cast<uint32_t>(value.size()));
            os.write(value.data(), static_cast<std::streamsize>(value.size()));
        };

        writeU32(0x41585342);
        writeU32(3);
        writeU32(1);
        writeString("ConfiglessV3Entity");
        writeString("Default");
        writeU32(1);
        writeBool(false);
        writeBool(false);
        writeBool(false);
        writeI32(-1);
        writeBool(false);
        writeBool(false);
        writeBool(false);
        writeBool(false);
        writeBool(false);
        writeBool(false);
        writeBool(false);
    }

    Scene loaded;
    BinarySceneSerializer serializer;
    AXIS_CHECK(serializer.Deserialize(path.string(), loaded));
    AXIS_CHECK(!configManager.GetConfig().debug.lightGizmos);

    axis_test_support::ResetServices();
}
