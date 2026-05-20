#pragma once

#include <resource/logic/resource_manager.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_serializer.h>
#include <entt/entt.hpp>
#include <functional>
#include <memory>

class Application;
class IPhysicsWorld;

using ComponentLoaderFunc =
    std::function<void(Scene&, entt::entity, const YAMLNode&, ResourceManager&, IPhysicsWorld*)>;

class ComponentLoader
{
public:
    static void RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory);
    static void RegisterLoader(const std::string& type, ComponentLoaderFunc func);

    static bool Load(const std::string& type, Scene& scene, entt::entity entity, const YAMLNode& node,
                     ResourceManager& res, IPhysicsWorld* phys);

    static void LoadRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadAnimator(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadLightDir(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadLightPoint(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadLightSpot(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadUITransform(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadUIRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadUIText(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadUIFlex(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadSkyboxRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadScript(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadAudioSource(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadVideoPlayer(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadParticleEmitter(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadMaterial(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadLOD(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadCamera(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadPathFollower(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadDecal(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadPostProcess(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadReflective(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadReflectionProbe(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadFragment(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadTransform(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadPlanarReflection(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadLightProbe(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadTerrain(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadNetwork(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void InitializeDefaultLoaders();

private:
    static std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> s_Factories;
    static std::unordered_map<std::string, ComponentLoaderFunc> s_Loaders;
};
