#pragma once

#include <core/engine_context.h>
#include <audio/interfaces/i_sound.h>
#include <resource/resource_manager.h>
#include <scene/scene.h>
#include <scene/scene_serializer.h>
#include <string>
#include <utils/yaml_parser.h>

class Application;

#include <functional>
#include <unordered_map>
#include <memory>

class IPhysicsWorld;

class IComponentLoaderFactory
{
public:
    virtual ~IComponentLoaderFactory() = default;
    virtual void Load(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld &phys, EngineContext ctx) = 0;
};

using ComponentLoaderFunc = std::function<void(Scene &, entt::entity, const YAMLNode &, ResourceManager &, IPhysicsWorld &, EngineContext)>;

class ComponentLoader
{
public:
    static void RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory);
    static void RegisterLoader(const std::string& type, ComponentLoaderFunc func);
    
    static bool Load(const std::string& type, Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld &phys, EngineContext ctx);

    // Existing hardcoded loaders, soon to be refactored or moved into the registry
    static void LoadRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadAnimator(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadLightDir(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadLightPoint(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadLightSpot(Scene &scene, entt::entity entity, const YAMLNode &node);

    static void LoadUITransform(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadUIRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadUIText(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);

    static void LoadSkyboxRenderer(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadScript(Scene &scene, entt::entity entity, const YAMLNode &node, EngineContext ctx);

    static void LoadAudioSource(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadVideoPlayer(Scene &scene, entt::entity entity, const YAMLNode &node);
    static void LoadParticleEmitter(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadMaterial(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadLOD(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res);
    static void LoadCamera(Scene &scene, entt::entity entity, const YAMLNode &node);

    static void InitializeDefaultLoaders();

private:
    static void ValidateKeys(const YAMLNode& node, const std::vector<std::string>& allowedKeys, const std::string& componentName);
    
    static std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> s_Factories;
    static std::unordered_map<std::string, ComponentLoaderFunc> s_Loaders;
};
