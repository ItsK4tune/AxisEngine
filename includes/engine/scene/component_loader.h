#pragma once

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <interface/audio/i_sound.h>
class Application;
#include <scene/scene_serializer.h>

class ComponentLoader
{
public:
    static void LoadRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadAnimator(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadLightDir(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadLightPoint(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadLightSpot(Scene &scene, entt::entity entity, const YAMLNode &node);

    static void LoadUITransform(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadUIRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadUIText(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);

    static void LoadSkyboxRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadScript(Scene &scene, entt::entity entity, const YAMLNode &node, std::shared_ptr<Application> app);

    static void LoadAudioSource(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadVideoPlayer(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadParticleEmitter(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadMaterial(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadLOD(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadCamera(Scene &scene, entt::entity entity, const YAMLNode &node);

private:
    static void ValidateKeys(const YAMLNode& node, const std::vector<std::string>& allowedKeys, const std::string& componentName);
};
