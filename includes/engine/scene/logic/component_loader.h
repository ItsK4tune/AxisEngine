#pragma once

#include <audio/interface/i_sound.h>
#include <core/logic/loader_utils.h>
#include <core/logic/yaml_parser.h>
#include <core/unit/engine_context.h>
#include <functional>
#include <memory>
#include <resource/manager/resource_manager.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_serializer.h>
#include <string>
#include <unordered_map>

class Application;
class IPhysicsWorld;

using ComponentLoaderFunc = std::function<void(Scene &, entt::entity, const YAMLNode &, ResourceManager &, IPhysicsWorld &, EngineContext)>;

class ComponentLoader
{
public:
    static void RegisterLoader(const std::string& type, std::shared_ptr<IComponentLoaderFactory> factory);
    static void RegisterLoader(const std::string& type, ComponentLoaderFunc func);
    
    static bool Load(const std::string& type, Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld &phys, EngineContext ctx);

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
    
    static std::unordered_map<std::string, std::shared_ptr<IComponentLoaderFactory>> s_Factories;
    static std::unordered_map<std::string, ComponentLoaderFunc> s_Loaders;
};