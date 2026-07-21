#pragma once

#include <scene/interface/i_component_loader_factory.h>
#include <scene/interface/i_component_serializer_factory.h>
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

class Application;
class IPhysicsWorld;
class ResourceManager;
struct Scene;

using ComponentLoaderFunc =
    std::function<void(Scene&, entt::entity, const YAMLNode&, ResourceManager&, IPhysicsWorld*)>;
using ComponentSerializerFunc = std::function<bool(const entt::registry&, entt::entity, YAMLNode&)>;

class FragmentLoader;

class ComponentLoader
{
public:
    static void RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory);
    static void RegisterLoader(const std::string& type, ComponentLoaderFunc func);
    static bool UnregisterLoader(const std::string& type);
    static void RegisterSerializer(const std::string& type, std::shared_ptr<IComponentSerializerFactory> factory);
    static void RegisterSerializer(const std::string& type, ComponentSerializerFunc func);
    static bool UnregisterSerializer(const std::string& type);
    static std::vector<std::pair<std::string, YAMLNode>> CollectSerializedComponents(const entt::registry& registry,
                                                                                     entt::entity entity);
    static bool HasSerializedComponents(const entt::registry& registry, entt::entity entity);

private:
    friend class SceneSerializer;
    friend class FragmentLoader;
    static bool Load(const std::string& type, Scene& scene, entt::entity entity, const YAMLNode& node,
                     ResourceManager& res, IPhysicsWorld* phys);
    static void InitializeDefaultLoaders();

    static void LoadRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadAnimator(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadLightDir(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadLightPoint(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadLightSpot(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadUITransform(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadUIRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadUIText(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadUIFlex(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadUIInteractive(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadUIAnimation(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadSkyboxRenderer(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadScript(Scene& scene, entt::entity entity, const YAMLNode& node);

    static void LoadAudioSource(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadVideoPlayer(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadParticleEmitter(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadMaterial(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadLOD(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res);
    static void LoadCamera(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadPathFollower(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadNavMesh(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadNavigationGrid(Scene& scene, entt::entity entity, const YAMLNode& node);

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
    static void LoadOcclusion(Scene& scene, entt::entity entity, const YAMLNode& node);
    static void LoadStreaming(Scene& scene, entt::entity entity, const YAMLNode& node);

    static std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> s_Factories;
    static std::unordered_map<std::string, std::shared_ptr<IComponentSerializerFactory>> s_Serializers;
    static std::shared_mutex s_RegistryMutex;
};
