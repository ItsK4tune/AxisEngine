#include <scene/logic/scene_serializer.h>
#include <scene/logic/binary_scene_serializer.h>
#include <core/logic/yaml_writer.h>
#include <audio/interface/i_audio_engine.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_loader.h>
#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/fragment_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/network_components.h>
#include <engine/platform/logic/io_handler.h>
#include <navigation/unit/pathfollower_component.h>
#include <navigation/unit/navmesh_component.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/physics_loader.h>
#include <platform/logic/monitor_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_load_finalizer.h>
#include <algorithm>
#include <iomanip>
#include <cctype>
#include <filesystem>
#include <set>
#include <unordered_set>

SceneSerializer::SceneSerializer(ResourceManager& res, IPhysicsWorld* phys, AudioService* audio)
    : m_Res(res), m_Phys(phys), m_Audio(audio)
{
}

bool SceneSerializer::Serialize(const std::string& filepath, const Scene& scene)
{
    return Serialize(filepath, scene, "");
}

bool SceneSerializer::Deserialize(const std::string& filepath, Scene& scene)
{
    SceneLoadResult dummy;
    return Deserialize(filepath, scene, dummy);
}

bool SceneSerializer::Deserialize(const std::string& filepath, Scene& scene, SceneLoadResult& outResult)
{
    std::string fullPath = FileSystem::getPath(filepath);
    auto roots = YAMLParser::Parse(fullPath);
    if (roots.empty())
    {
        if (!std::filesystem::exists(fullPath))
        {
            LOGGER_ERROR("SceneSerializer") << "AXS file does not exist: " << fullPath;
        }
        else
        {
            LOGGER_ERROR("SceneSerializer") << "Failed to parse AXS file (empty or malformed): " << fullPath;
        }
        outResult = {};
        return false;
    }

    return DeserializeNodes(std::move(roots), filepath, fullPath, scene, outResult);
}

bool SceneSerializer::DeserializeFromString(const std::string& content, const std::string& sourceName, Scene& scene,
                                            SceneLoadResult& outResult)
{
    auto roots = YAMLParser::ParseString(content);
    if (roots.empty())
    {
        LOGGER_ERROR("SceneSerializer") << "Failed to parse in-memory AXS scene: " << sourceName;
        outResult = {};
        return false;
    }
    return DeserializeNodes(std::move(roots), sourceName, sourceName, scene, outResult);
}

bool SceneSerializer::DeserializeNodes(std::vector<YAMLNode> roots, const std::string& sourceName,
                                       const std::string& sourceDisplay, Scene& scene, SceneLoadResult& outResult)
{
    ResourceManager& res = m_Res;
    IPhysicsWorld* phys = m_Phys;
    AudioService* sound = m_Audio;
    SceneLoadResult result;

    std::string sceneName = sourceName;
    size_t slash = sceneName.find_last_of("/\\");
    if (slash != std::string::npos)
        sceneName = sceneName.substr(slash + 1);
    auto dotPos = sceneName.rfind('.');
    if (dotPos != std::string::npos)
        sceneName = sceneName.substr(0, dotPos);

    for (auto& root : roots)
    {
        if (root.key.rfind("axis_", 0) == 0 && root.key != "axis_scene")
        {
            LOGGER_WARN("SceneSerializer")
                << "Potential typo in root key: '" << root.key << "', expected 'axis_scene' in " << sourceDisplay;
        }
    }

    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    std::vector<YAMLNode> activeRoots = roots;
    if (roots.size() == 1 && roots[0].key == "axis_scene")
    {
        activeRoots = roots[0].children;
    }
    for (auto& root : activeRoots)
    {
        if (root.key == "Resources")
        {
            for (auto& resNode : root.children)
            {
                if (resNode.key == "Shader")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string vs =
                        resNode.GetChildValue("VS", resNode.GetChildValue("vertex", resNode.GetChildValue("Vertex")));
                    std::string fs = resNode.GetChildValue(
                        "FS", resNode.GetChildValue("fragment", resNode.GetChildValue("Fragment")));
                    std::string gs = resNode.GetChildValue(
                        "GS", resNode.GetChildValue("geometry", resNode.GetChildValue("Geometry")));
                    res.LoadShader(name, vs, fs, gs);
                    result.loadedShaders.push_back(name);
                }
                else if (resNode.key == "Model")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    bool isStatic = resNode.GetChildValue("Static") == "true" || resNode.GetChildValue("Static") == "1";
                    res.LoadModel(name, path, isStatic);
                    result.loadedModels.push_back(name);
                }
                else if (resNode.key == "Texture")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    res.LoadTexture(name, path);
                    result.loadedTextures.push_back(name);
                }
                else if (resNode.key == "Font")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    unsigned int size = LoaderUtils::SafeStoul(resNode.GetChildValue("Size", "16"));
                    res.LoadFont(name, path, size);
                    result.loadedFonts.push_back(name);
                }
                else if (resNode.key == "Skybox")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::vector<std::string> faces = {resNode.GetChildValue("Right"), resNode.GetChildValue("Left"),
                                                      resNode.GetChildValue("Top"),   resNode.GetChildValue("Bottom"),
                                                      resNode.GetChildValue("Front"), resNode.GetChildValue("Back")};
                    res.LoadSkybox(name, faces);
                    result.loadedSkyboxes.push_back(name);
                }
                else if (resNode.key == "Animation")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    std::string model = resNode.GetChildValue("Model");
                    res.LoadAnimation(name, path, model);
                    result.loadedAnimations.push_back(name);
                }
                else if (resNode.key == "Audio" || resNode.key == "Sound")
                {
                    if (sound)
                    {
                        std::string name = resNode.GetChildValue("Name");
                        std::string path = resNode.GetChildValue("Path");
                        res.LoadSound(name, path);
                        result.loadedSounds.push_back(name);
                    }
                }
                else
                {
                    LOGGER_WARN("SceneSerializer")
                        << "Unknown resource type '" << resNode.key << "' in Resources block";
                }
            }
        }
        else if (root.key == "Entities")
        {
            ComponentLoader::InitializeDefaultLoaders();
            std::function<void(YAMLNode&, entt::entity)> ParseEntity = [&](YAMLNode& entNode, entt::entity parent) {
                std::string entityName = entNode.key;
                entt::entity currentEntity = scene.GetRegistry().create();
                scene.AddComponent<PositionComponent>(currentEntity);
                scene.AddComponent<RotationComponent>(currentEntity);
                scene.AddComponent<ScaleComponent>(currentEntity);
                scene.AddComponent<HierarchyComponent>(currentEntity);
                scene.AddComponent<WorldTransformComponent>(currentEntity);
                scene.AddComponent<InfoComponent>(currentEntity, entityName, entNode.GetChildValue("Tag", "default"));

                auto& info = scene.GetComponent<InfoComponent>(currentEntity);
                info.sceneName = entNode.GetChildValue("Scene", entNode.GetChildValue("SceneName", sceneName));
                if (auto* layerNode = entNode.GetChild("Layer"))
                {
                    info.layer = static_cast<uint32_t>(LoaderUtils::SafeStoul(layerNode->value, info.layer));
                }
                const std::string activeValue = entNode.GetChildValue("Active", "true");
                info.isActive = activeValue == "true" || activeValue == "1";
                info.renderOrder = LoaderUtils::SafeStoi(entNode.GetChildValue("RenderOrder", "0"));
                result.entities.push_back(currentEntity);

                if (parent != entt::null)
                {
                    auto& h = scene.GetComponent<HierarchyComponent>(currentEntity);
                    h.parent = parent;
                    scene.GetComponent<HierarchyComponent>(parent).children.push_back(currentEntity);
                }
                else if (auto* pNode = entNode.GetChild("Parent"))
                {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto& child : entNode.children)
                {
                    if (child.key == "Component")
                    {
                        if (!ComponentLoader::Load(child.value, scene, currentEntity, child, res, phys))
                            LOGGER_WARN("SceneSerializer")
                                << "Unknown component type '" << child.value << "' on entity '" << entityName << "'";
                    }
                    else if (child.key != "Tag" && child.key != "Layer" && child.key != "Active" &&
                             child.key != "RenderOrder" && child.key != "Parent" && child.key != "Scene" &&
                             child.key != "SceneName")
                    {
                        ParseEntity(child, currentEntity);
                    }
                }
            };

            for (auto& entNode : root.children) ParseEntity(entNode, entt::null);
        }
        else
        {
            LOGGER_WARN("SceneSerializer") << "Unknown root block '" << root.key << "' in axis_scene";
        }
    }

    auto followerView = scene.View<PathFollowerComponent>();
    for (auto entity : followerView)
    {
        auto& follower = followerView.get<PathFollowerComponent>(entity);
        if (!follower.navigationProviderName.empty())
            follower.navigationProviderEntity = scene.FindByName(follower.navigationProviderName);
    }

    if (!SceneHandlers::SceneLoadFinalizer::Finalize(scene, result, phys, deferredChildren))
    {
        outResult = std::move(result);
        return false;
    }

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS scene: " << sourceDisplay;
    outResult = std::move(result);
    return !outResult.entities.empty();
}

// ========== Phase 4: Scene Serialization ==========

#include <ecs/unit/decal_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/ui_components.h>
#include <fstream>
#include <sstream>

std::string SceneSerializer::NormalizeSceneName(const std::string& name)
{
    std::string n = name;
    size_t slash = n.find_last_of("/\\");
    if (slash != std::string::npos)
        n = n.substr(slash + 1);
    size_t dot = n.rfind('.');
    if (dot != std::string::npos)
        n = n.substr(0, dot);
    std::transform(n.begin(), n.end(), n.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return n;
}

std::string SceneSerializer::NormalizePath(const std::string& path)
{
    if (path.empty())
        return "";
    return FileSystem::getRelativePath(path);
}

static std::string FloatStr(float f)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << f;
    return ss.str();
}

static std::string Vec2Str(const glm::vec2& v)
{
    std::ostringstream ss;
    ss << FloatStr(v.x) << " " << FloatStr(v.y);
    return ss.str();
}

static std::string Vec3Str(const glm::vec3& v)
{
    std::ostringstream ss;
    ss << FloatStr(v.x) << " " << FloatStr(v.y) << " " << FloatStr(v.z);
    return ss.str();
}

static std::string Vec4Str(const glm::vec4& v)
{
    std::ostringstream ss;
    ss << FloatStr(v.x) << " " << FloatStr(v.y) << " " << FloatStr(v.z) << " " << FloatStr(v.w);
    return ss.str();
}

#define SerialWriteKV(f, indent, key, val)              \
    do                                                  \
    {                                                   \
        for (int i = 0; i < (indent); ++i) (f) << "  "; \
        (f) << (key) << ": " << (val) << "\n";          \
    } while (0)

static std::string Vec2PercentStr(const glm::vec2& v, const glm::bvec2& p)
{
    std::ostringstream ss;
    ss << FloatStr(v.x);
    if (p.x)
        ss << "%";
    ss << " " << FloatStr(v.y);
    if (p.y)
        ss << "%";
    return ss.str();
}

static void WriteComponentHeader(std::ostream& f, int indent, const std::string& type)
{
    for (int i = 0; i < indent; ++i) f << "  ";
    f << "Component: " << type << "\n";
}

static bool IsBuiltInComponentType(const std::string& type)
{
    static const std::unordered_set<std::string> types = {
        "Animator",       "AudioSource",       "Camera",           "CharacterController",
        "Decal",         "DirectionalLight",  "Fragment",         "LightProbe",
        "LOD",           "Material",          "NavMesh",          "NavigationGrid",
        "Network",       "Occlusion",         "ParticleEmitter",  "PathFollower",
        "PlanarReflection", "PointLight",      "PostProcess",      "ReflectionProbe",
        "Reflective",    "Renderer",          "RigidBody",        "RigidShape",
        "Script",        "SkyboxRenderer",    "SpotLight",        "Streaming",
        "Terrain",       "Transform",         "UIAnimation",      "UIFlex",
        "UIInteractive", "UIRenderer",        "UIText",           "UITransform",
        "VideoPlayer"};
    return types.contains(type);
}

struct UsedResources
{
    std::set<std::string> shaders;
    std::set<std::string> models;
    std::set<std::string> textures;
    std::set<std::string> fonts;
    std::set<std::string> skyboxes;
    std::set<std::string> animations;
    std::set<std::string> sounds;
};

static void CollectResources(entt::registry& reg, entt::entity entity, UsedResources& ur, const std::string& sceneName)
{
    if (auto* info = reg.try_get<InfoComponent>(entity))
    {
        if (!sceneName.empty() &&
            SceneSerializer::NormalizeSceneName(info->sceneName) != SceneSerializer::NormalizeSceneName(sceneName))
            return;
    }

    if (auto* mr = reg.try_get<MeshRendererComponent>(entity))
    {
        if (!mr->shaderName.empty())
            ur.shaders.insert(mr->shaderName);
        if (!reg.all_of<StreamingComponent>(entity))
        {
            if (mr->model)
                ur.models.insert(mr->model->GetName());
            else if (!mr->modelName.empty())
                ur.models.insert(mr->modelName);
        }
    }
    if (auto* mat = reg.try_get<MaterialComponent>(entity))
    {
        if (!mat->desc.albedoPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.albedoPath));
        if (!mat->desc.normalPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.normalPath));
        if (!mat->desc.metallicPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.metallicPath));
        if (!mat->desc.roughnessPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.roughnessPath));
        if (!mat->desc.aoPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.aoPath));
        if (!mat->desc.emissivePath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.emissivePath));
        if (!mat->desc.specularPath.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.specularPath));
    }
    if (auto* anim = reg.try_get<AnimationComponent>(entity))
    {
        for (auto& a : anim->animations) ur.animations.insert(a);
    }
    if (auto* pe = reg.try_get<ParticleEmitterComponent>(entity))
    {
        if (!pe->textureName.empty())
            ur.textures.insert(pe->textureName);
        if (!pe->customShader.empty())
            ur.shaders.insert(pe->customShader);
    }
    if (auto* sky = reg.try_get<SkyboxRenderComponent>(entity))
    {
        if (sky->skybox)
            ur.skyboxes.insert(sky->skybox->GetName());
        else if (!sky->skyboxName.empty())
            ur.skyboxes.insert(sky->skyboxName);
        if (!sky->shaderName.empty())
            ur.shaders.insert(sky->shaderName);
    }
    if (auto* uir = reg.try_get<UIRendererComponent>(entity))
    {
        if (uir->texture)
            ur.textures.insert(SceneSerializer::NormalizePath(uir->texture->path));
        else if (!uir->textureName.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(uir->textureName));
        if (!uir->shaderName.empty())
            ur.shaders.insert(uir->shaderName);
    }
    if (auto* uit = reg.try_get<UITextComponent>(entity))
    {
        if (!uit->fontName.empty())
            ur.fonts.insert(uit->fontName);
        if (!uit->shaderName.empty())
            ur.shaders.insert(uit->shaderName);
    }
    if (auto* pp = reg.try_get<PostProcessComponent>(entity))
    {
        for (auto& eff : pp->effects) ur.shaders.insert(eff.shaderName);
    }
    if (auto* audio = reg.try_get<AudioSourceComponent>(entity))
    {
        if (!audio->resourceName.empty())
            ur.sounds.insert(audio->resourceName);
    }
    if (auto* decal = reg.try_get<DecalComponent>(entity))
    {
        if (!decal->albedoTexture.empty())
            ur.textures.insert(SceneSerializer::NormalizePath(decal->albedoTexture));
        if (!decal->customShader.empty())
            ur.shaders.insert(decal->customShader);
    }
    if (auto* terrain = reg.try_get<TerrainComponent>(entity))
    {
        if (!terrain->heightMapName.empty())
            ur.textures.insert(terrain->heightMapName);
        if (!terrain->splatMapName.empty())
            ur.textures.insert(terrain->splatMapName);
        ur.textures.insert(terrain->diffuseLayerNames.begin(), terrain->diffuseLayerNames.end());
        ur.textures.insert(terrain->normalLayerNames.begin(), terrain->normalLayerNames.end());
        if (!terrain->customShader.empty())
            ur.shaders.insert(terrain->customShader);
    }
    if (auto* lod = reg.try_get<LODComponent>(entity))
    {
        for (size_t i = 0; i < std::max(lod->lodModels.size(), lod->lodModelNames.size()); ++i)
        {
            if (i < lod->lodModels.size() && lod->lodModels[i])
                ur.models.insert(lod->lodModels[i]->GetName());
            else if (i < lod->lodModelNames.size() && !lod->lodModelNames[i].empty())
                ur.models.insert(lod->lodModelNames[i]);
        }
    }
    // Recursively visit children
    if (auto* h = reg.try_get<HierarchyComponent>(entity))
    {
        for (auto child : h->children)
        {
            CollectResources(reg, child, ur, sceneName);
        }
    }
}

static void SerializeEntity(std::ostream& f, entt::registry& reg, entt::entity entity, int indent,
                            const std::string& sceneName)
{
    auto* info = reg.try_get<InfoComponent>(entity);
    std::string name = info ? info->name : ("Entity_" + std::to_string((uint32_t)entity));
    std::string targetScene = SceneSerializer::NormalizeSceneName(sceneName);

    if (info && info->isTransient)
        return;

    if (info && !targetScene.empty() && SceneSerializer::NormalizeSceneName(info->sceneName) != targetScene)
        return;

    for (int i = 0; i < indent; ++i) f << "  ";
    f << name << ":\n";
    int ci = indent + 1, ti = ci + 1;

    if (info && !info->tag.empty() && info->tag != "default")
        SerialWriteKV(f, ci, "Tag", info->tag);
    if (info && !info->isActive)
        SerialWriteKV(f, ci, "Active", "false");
    if (info && info->layer != 0)
        SerialWriteKV(f, ci, "Layer", std::to_string(info->layer));
    if (info && info->renderOrder != 0)
        SerialWriteKV(f, ci, "RenderOrder", std::to_string(info->renderOrder));
    if (info && targetScene.empty() && !info->sceneName.empty())
        SerialWriteKV(f, ci, "Scene", info->sceneName);

    // Parent link if parent is in a different scene
    if (auto* h = reg.try_get<HierarchyComponent>(entity))
    {
        if (h->parent != entt::null)
        {
            if (auto* pInfo = reg.try_get<InfoComponent>(h->parent))
            {
                if (!targetScene.empty() && SceneSerializer::NormalizeSceneName(pInfo->sceneName) != targetScene)
                {
                    SerialWriteKV(f, ci, "Parent", pInfo->name);
                }
            }
        }
    }

    // Transform
    auto* pos = reg.try_get<PositionComponent>(entity);
    auto* rot = reg.try_get<RotationComponent>(entity);
    auto* scale = reg.try_get<ScaleComponent>(entity);
    if (pos || rot || scale)
    {
        WriteComponentHeader(f, ci, "Transform");
        if (pos)
            SerialWriteKV(f, ti, "Position", Vec3Str(pos->value));
        if (rot)
            SerialWriteKV(f, ti, "Rotation", Vec3Str(glm::degrees(glm::eulerAngles(rot->value))));
        if (scale)
            SerialWriteKV(f, ti, "Scale", Vec3Str(scale->value));
    }

    if (auto* camera = reg.try_get<CameraComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Camera");
        SerialWriteKV(f, ti, "Primary", camera->isPrimary ? "true" : "false");
        SerialWriteKV(f, ti, "FOV", FloatStr(camera->fov));
        SerialWriteKV(f, ti, "Near", FloatStr(camera->nearPlane));
        SerialWriteKV(f, ti, "Far", FloatStr(camera->farPlane));
        SerialWriteKV(f, ti, "AspectRatio", FloatStr(camera->aspectRatio));
        SerialWriteKV(f, ti, "ScreenWidth", std::to_string(camera->screenWidth));
        SerialWriteKV(f, ti, "ScreenHeight", std::to_string(camera->screenHeight));
        SerialWriteKV(f, ti, "Orthographic", camera->isOrthographic ? "true" : "false");
        SerialWriteKV(f, ti, "OrthoSize", FloatStr(camera->orthoSize));
        SerialWriteKV(f, ti, "CullingMask", std::to_string(camera->cullingMask));
    }

    if (auto* mr = reg.try_get<MeshRendererComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Renderer");
        if (!reg.all_of<StreamingComponent>(entity))
            SerialWriteKV(f, ti, "Model", mr->model ? mr->model->GetName() : mr->modelName);
        if (!mr->shaderName.empty())
            SerialWriteKV(f, ti, "Shader", mr->shaderName);
        SerialWriteKV(f, ti, "Order", std::to_string(mr->order));
        SerialWriteKV(f, ti, "CastShadow", mr->castShadow ? "true" : "false");
        SerialWriteKV(f, ti, "ReceiveShadow", mr->receiveShadow ? "true" : "false");
        SerialWriteKV(f, ti, "IgnoreDepth", mr->ignoreDepth ? "true" : "false");
        SerialWriteKV(f, ti, "Color", Vec4Str(mr->color));
        int renderModeValue = (mr->renderMode == RenderMode::ForceForward) ? (int)RenderMode::ForceForward : 0;
        SerialWriteKV(f, ti, "RenderMode", std::to_string(renderModeValue));
    }

    if (auto* mat = reg.try_get<MaterialComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Material");
        SerialWriteKV(f, ti, "Opacity", FloatStr(mat->desc.opacity));
        SerialWriteKV(f, ti, "Roughness", FloatStr(mat->desc.pbr.roughness));
        SerialWriteKV(f, ti, "Metallic", FloatStr(mat->desc.pbr.metallic));
        SerialWriteKV(f, ti, "AO", FloatStr(mat->desc.pbr.ao));
        SerialWriteKV(f, ti, "AlphaCutoff", FloatStr(mat->desc.alphaCutoff));
        SerialWriteKV(f, ti, "Emission", Vec3Str(mat->desc.emission));
        SerialWriteKV(f, ti, "UVScale", Vec2Str(mat->desc.uvScale));
        SerialWriteKV(f, ti, "UVOffset", Vec2Str(mat->desc.uvOffset));

        SerialWriteKV(f, ti, "BlendSrc", std::to_string(static_cast<int>(mat->desc.blendSrc)));
        SerialWriteKV(f, ti, "BlendDst", std::to_string(static_cast<int>(mat->desc.blendDst)));
        SerialWriteKV(f, ti, "Type", mat->desc.type);
        std::ostringstream ports;
        for (size_t i = 0; i < std::size(mat->desc.ports.data); ++i)
        {
            if (i != 0)
                ports << ' ';
            ports << FloatStr(mat->desc.ports.data[i]);
        }
        SerialWriteKV(f, ti, "Ports", ports.str());

        if (!mat->desc.albedoPath.empty())
            SerialWriteKV(f, ti, "Albedo", SceneSerializer::NormalizePath(mat->desc.albedoPath));
        if (!mat->desc.normalPath.empty())
            SerialWriteKV(f, ti, "Normal", SceneSerializer::NormalizePath(mat->desc.normalPath));
        if (!mat->desc.metallicPath.empty())
            SerialWriteKV(f, ti, "MetallicMap", SceneSerializer::NormalizePath(mat->desc.metallicPath));
        if (!mat->desc.roughnessPath.empty())
            SerialWriteKV(f, ti, "RoughnessMap", SceneSerializer::NormalizePath(mat->desc.roughnessPath));
        if (!mat->desc.aoPath.empty())
            SerialWriteKV(f, ti, "AO_Map", SceneSerializer::NormalizePath(mat->desc.aoPath));
        if (!mat->desc.emissivePath.empty())
            SerialWriteKV(f, ti, "EmissiveMap", SceneSerializer::NormalizePath(mat->desc.emissivePath));
        if (!mat->desc.specularPath.empty())
            SerialWriteKV(f, ti, "SpecularMap", SceneSerializer::NormalizePath(mat->desc.specularPath));
    }

    // Animator
    if (auto* anim = reg.try_get<AnimationComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Animator");
        std::string anims;
        for (auto& a : anim->animations) anims += a + " ";
        if (!anims.empty())
            SerialWriteKV(f, ti, "Animation", anims);
        SerialWriteKV(f, ti, "Speed", FloatStr(anim->speed));
        SerialWriteKV(f, ti, "StartTime", FloatStr(anim->startTime));
        SerialWriteKV(f, ti, "Rate", FloatStr(anim->rate));
        SerialWriteKV(f, ti, "BlendFactor", FloatStr(anim->blendFactor));
        SerialWriteKV(f, ti, "GraphEnabled", anim->graph.enabled ? "true" : "false");
        if (anim->graph.entryState != 0)
            SerialWriteKV(f, ti, "GraphEntry", std::to_string(anim->graph.entryState));
        for (const auto& parameter : anim->graph.parameters)
        {
            std::ostringstream record;
            record << std::quoted(parameter.name) << " " << static_cast<int>(parameter.type) << " "
                   << FloatStr(parameter.floatValue) << " " << parameter.boolValue << " " << parameter.triggerValue;
            SerialWriteKV(f, ti, "GraphParameter", record.str());
        }
        for (const auto& state : anim->graph.states)
        {
            std::ostringstream record;
            record << state.id << " " << std::quoted(state.name) << " " << std::quoted(state.clip) << " "
                   << FloatStr(state.speed) << " " << FloatStr(state.editorPosition.x) << " "
                   << FloatStr(state.editorPosition.y);
            SerialWriteKV(f, ti, "GraphState", record.str());
        }
        for (const auto& transition : anim->graph.transitions)
        {
            std::ostringstream record;
            record << transition.id << " " << transition.fromState << " " << transition.toState << " "
                   << FloatStr(transition.duration) << " " << transition.hasExitTime << " "
                   << FloatStr(transition.exitTime) << " " << static_cast<int>(transition.conditionLogic) << " "
                   << transition.conditions.size();
            for (const auto& condition : transition.conditions)
                record << " " << std::quoted(condition.parameter) << " " << static_cast<int>(condition.op) << " "
                       << FloatStr(condition.threshold) << " " << condition.negated;
            SerialWriteKV(f, ti, "GraphTransitionV2", record.str());
        }
    }

    // Lights
    if (auto* l = reg.try_get<DirectionalLightComponent>(entity))
    {
        WriteComponentHeader(f, ci, "DirectionalLight");
        SerialWriteKV(f, ti, "Active", l->active ? "true" : "false");
        SerialWriteKV(f, ti, "CastShadow", l->isCastShadow ? "true" : "false");
        SerialWriteKV(f, ti, "Direction", Vec3Str(l->direction));
        SerialWriteKV(f, ti, "Color", Vec3Str(l->color));
        SerialWriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        SerialWriteKV(f, ti, "Ambient", FloatStr(l->ambient));
        SerialWriteKV(f, ti, "Diffuse", FloatStr(l->diffuse));
        SerialWriteKV(f, ti, "Specular", FloatStr(l->specular));
    }
    if (auto* l = reg.try_get<PointLightComponent>(entity))
    {
        WriteComponentHeader(f, ci, "PointLight");
        SerialWriteKV(f, ti, "Active", l->active ? "true" : "false");
        SerialWriteKV(f, ti, "CastShadow", l->isCastShadow ? "true" : "false");
        SerialWriteKV(f, ti, "Color", Vec3Str(l->color));
        SerialWriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        SerialWriteKV(f, ti, "Radius", FloatStr(l->radius));
        SerialWriteKV(f, ti, "Constant", FloatStr(l->constant));
        SerialWriteKV(f, ti, "Linear", FloatStr(l->linear));
        SerialWriteKV(f, ti, "Quadratic", FloatStr(l->quadratic));
        SerialWriteKV(f, ti, "Ambient", FloatStr(l->ambient));
        SerialWriteKV(f, ti, "Diffuse", FloatStr(l->diffuse));
        SerialWriteKV(f, ti, "Specular", FloatStr(l->specular));
    }
    if (auto* l = reg.try_get<SpotLightComponent>(entity))
    {
        WriteComponentHeader(f, ci, "SpotLight");
        SerialWriteKV(f, ti, "Active", l->active ? "true" : "false");
        SerialWriteKV(f, ti, "CastShadow", l->isCastShadow ? "true" : "false");
        SerialWriteKV(f, ti, "Direction", Vec3Str(l->direction));
        SerialWriteKV(f, ti, "Color", Vec3Str(l->color));
        SerialWriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        SerialWriteKV(f, ti, "Radius", FloatStr(l->radius));
        SerialWriteKV(f, ti, "CutOff", FloatStr(glm::degrees(glm::acos(l->cutOff))));
        SerialWriteKV(f, ti, "OuterCutOff", FloatStr(glm::degrees(glm::acos(l->outerCutOff))));
        SerialWriteKV(f, ti, "Constant", FloatStr(l->constant));
        SerialWriteKV(f, ti, "Linear", FloatStr(l->linear));
        SerialWriteKV(f, ti, "Quadratic", FloatStr(l->quadratic));
        SerialWriteKV(f, ti, "Ambient", FloatStr(l->ambient));
        SerialWriteKV(f, ti, "Diffuse", FloatStr(l->diffuse));
        SerialWriteKV(f, ti, "Specular", FloatStr(l->specular));
    }

    // Physics
    if (auto* rs = reg.try_get<RigidShapeComponent>(entity))
    {
        WriteComponentHeader(f, ci, "RigidShape");
        SerialWriteKV(f, ti, "Type", ShapeTypeToString(rs->type));
        SerialWriteKV(f, ti, "Size", Vec3Str(rs->size));
        SerialWriteKV(f, ti, "Radius", FloatStr(rs->radius));
        SerialWriteKV(f, ti, "Height", FloatStr(rs->height));
        SerialWriteKV(f, ti, "Friction", FloatStr(rs->friction));
        SerialWriteKV(f, ti, "Restitution", FloatStr(rs->restitution));
        if (glm::length(rs->offset) > 0.0001f)
            SerialWriteKV(f, ti, "Offset", Vec3Str(rs->offset));
        glm::vec3 euler = glm::degrees(glm::eulerAngles(rs->rotation));
        if (glm::length(euler) > 0.0001f)
            SerialWriteKV(f, ti, "Rotation", Vec3Str(euler));
        if (!rs->children.empty())
        {
            f << std::string(ti * 2, ' ') << "Shapes:\n";
            for (const auto& child : rs->children)
            {
                f << std::string((ti + 1) * 2, ' ') << "Shape:\n";
                SerialWriteKV(f, ti + 2, "Type", ShapeTypeToString(child.type));
                SerialWriteKV(f, ti + 2, "Position", Vec3Str(child.position));
                SerialWriteKV(f, ti + 2, "Rotation", Vec3Str(glm::degrees(glm::eulerAngles(child.rotation))));
                SerialWriteKV(f, ti + 2, "Size", Vec3Str(child.size));
                SerialWriteKV(f, ti + 2, "Radius", FloatStr(child.radius));
                SerialWriteKV(f, ti + 2, "Height", FloatStr(child.height));
            }
        }
    }
    if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
    {
        WriteComponentHeader(f, ci, "RigidBody");
        SerialWriteKV(f, ti, "Mass", FloatStr(rb->mass));
        SerialWriteKV(f, ti, "BodyType", rb->isStatic ? "STATIC" : (rb->isKinematic ? "KINEMATIC" : "DYNAMIC"));
        SerialWriteKV(f, ti, "LinearDamping", FloatStr(rb->linearDamping));
        SerialWriteKV(f, ti, "AngularDamping", FloatStr(rb->angularDamping));
        SerialWriteKV(f, ti, "LinearFactor", Vec3Str(rb->linearFactor));
        SerialWriteKV(f, ti, "AngularFactor", Vec3Str(rb->angularFactor));
        SerialWriteKV(f, ti, "AttachToParent", rb->isAttachedToParent ? "true" : "false");
        SerialWriteKV(f, ti, "ParentMatter", rb->isParentMatter ? "true" : "false");
        SerialWriteKV(f, ti, "ChildrenMatter", rb->isChildrenMatter ? "true" : "false");
        SerialWriteKV(f, ti, "CollisionEnabled", rb->isCollisionEnabled ? "true" : "false");
        const glm::vec3 linearVelocity = rb->body ? rb->body->GetLinearVelocity() : rb->initialLinearVelocity;
        const glm::vec3 angularVelocity = rb->body ? rb->body->GetAngularVelocity() : rb->initialAngularVelocity;
        if (glm::length(linearVelocity) > 0.0001f)
            SerialWriteKV(f, ti, "LinearVelocity", Vec3Str(linearVelocity));
        if (glm::length(angularVelocity) > 0.0001f)
            SerialWriteKV(f, ti, "AngularVelocity", Vec3Str(angularVelocity));
        if (rb->isTrigger)
            SerialWriteKV(f, ti, "IsTrigger", "true");
    }

    if (auto* cc = reg.try_get<CharacterControllerComponent>(entity))
    {
        WriteComponentHeader(f, ci, "CharacterController");
        SerialWriteKV(f, ti, "Radius", FloatStr(cc->radius));
        SerialWriteKV(f, ti, "Height", FloatStr(cc->height));
        SerialWriteKV(f, ti, "StepHeight", FloatStr(cc->stepHeight));
        SerialWriteKV(f, ti, "MaxSlope", FloatStr(cc->maxSlope));
    }

    if (auto* frag = reg.try_get<FragmentComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Fragment");
        SerialWriteKV(f, ti, "Path", SceneSerializer::NormalizePath(frag->path));
        if (!frag->overrides.empty())
        {
            // Parse the override string back into nodes for structural serialization
            auto overrideRoots = YAMLParser::ParseString(frag->overrides);
            if (!overrideRoots.empty())
            {
                f << std::string(ti * 2, ' ') << "Overrides:\n";
                YAMLWriter::Write(f, overrideRoots, (ti + 1) * YAMLWriter::IndentWidth);
            }
        }
    }

    if (auto* audio = reg.try_get<AudioSourceComponent>(entity))
    {
        WriteComponentHeader(f, ci, "AudioSource");
        if (!audio->resourceName.empty())
            SerialWriteKV(f, ti, "Audio", audio->resourceName);
        else if (!audio->filePath.empty())
            SerialWriteKV(f, ti, "Path", SceneSerializer::NormalizePath(audio->filePath));
        SerialWriteKV(f, ti, "PlayOnAwake", audio->playOnAwake ? "true" : "false");
        SerialWriteKV(f, ti, "Loop", audio->loop ? "true" : "false");
        SerialWriteKV(f, ti, "Volume", FloatStr(audio->volume));
        SerialWriteKV(f, ti, "Pitch", FloatStr(audio->pitch));
        SerialWriteKV(f, ti, "Pan", FloatStr(audio->pan));
        SerialWriteKV(f, ti, "Speed", FloatStr(audio->speed));
        SerialWriteKV(f, ti, "Is3d", audio->is3D ? "true" : "false");
        SerialWriteKV(f, ti, "MinDistance", FloatStr(audio->minDistance));
        SerialWriteKV(f, ti, "MaxDistance", FloatStr(audio->maxDistance));
        SerialWriteKV(f, ti, "Velocity", Vec3Str(audio->velocity));
    }

    if (auto* video = reg.try_get<VideoPlayerComponent>(entity))
    {
        WriteComponentHeader(f, ci, "VideoPlayer");
        SerialWriteKV(f, ti, "Path", SceneSerializer::NormalizePath(video->filePath));
        SerialWriteKV(f, ti, "Loop", video->isLooping ? "true" : "false");
        SerialWriteKV(f, ti, "Speed", FloatStr(video->speed));
        SerialWriteKV(f, ti, "PlayOnAwake", video->playOnAwake ? "true" : "false");
        SerialWriteKV(f, ti, "Volume", FloatStr(video->volume));
        SerialWriteKV(f, ti, "MaxDecodes", std::to_string(video->maxDecodes));
    }

    if (auto* pe = reg.try_get<ParticleEmitterComponent>(entity))
    {
        WriteComponentHeader(f, ci, "ParticleEmitter");
        SerialWriteKV(f, ti, "Active", pe->isActive ? "true" : "false");
        if (!pe->textureName.empty())
            SerialWriteKV(f, ti, "Texture", pe->textureName);
        if (!pe->customShader.empty())
            SerialWriteKV(f, ti, "Shader", pe->customShader);
        SerialWriteKV(f, ti, "SpawnRate", FloatStr(pe->emitter.SpawnRate));
        SerialWriteKV(f, ti, "Lifetime", FloatStr(pe->emitter.LifeTime));
        SerialWriteKV(f, ti, "Gravity", Vec3Str(pe->emitter.Gravity));
        SerialWriteKV(f, ti, "Drag", FloatStr(pe->emitter.Drag));
        SerialWriteKV(f, ti, "GraphEnabled", pe->graph.enabled ? "true" : "false");
        for (const auto& parameter : pe->graph.parameters)
        {
            std::ostringstream record;
            record << std::quoted(parameter.name) << " " << static_cast<int>(parameter.type) << " "
                   << FloatStr(parameter.floatValue) << " " << parameter.boolValue << " " << parameter.triggerValue;
            SerialWriteKV(f, ti, "GraphParameter", record.str());
        }
        for (const auto& node : pe->graph.nodes)
        {
            std::ostringstream record;
            record << node.id << " " << static_cast<int>(node.type) << " " << std::quoted(node.name) << " "
                   << node.enabled << " " << FloatStr(node.scalarA) << " " << FloatStr(node.scalarB) << " "
                   << Vec4Str(node.valueA) << " " << Vec4Str(node.valueB) << " "
                   << FloatStr(node.editorPosition.x) << " " << FloatStr(node.editorPosition.y);
            SerialWriteKV(f, ti, "GraphNode", record.str());
        }
        for (const auto& link : pe->graph.links)
        {
            std::ostringstream record;
            record << link.id << " " << link.fromNode << " " << link.toNode << " "
                   << static_cast<int>(link.conditionLogic) << " " << link.conditions.size();
            for (const auto& condition : link.conditions)
                record << " " << std::quoted(condition.parameter) << " " << static_cast<int>(condition.op) << " "
                       << FloatStr(condition.threshold) << " " << condition.negated;
            SerialWriteKV(f, ti, "GraphLinkV2", record.str());
        }
        SerialWriteKV(f, ti, "Shape",
                      pe->emitter.Shape == ParticleEmitter::EmissionShape::CONE
                          ? "CONE"
                          : (pe->emitter.Shape == ParticleEmitter::EmissionShape::FIGURE_EIGHT ? "FIGURE_EIGHT"
                                                                                              : "DIRECTIONAL"));
        SerialWriteKV(f, ti, "Duration", FloatStr(pe->emissionDuration));
        SerialWriteKV(f, ti, "StartSize", FloatStr(pe->emitter.StartSize));
        SerialWriteKV(f, ti, "EndSize", FloatStr(pe->emitter.EndSize));
        SerialWriteKV(f, ti, "StartColor", Vec4Str(pe->emitter.StartColor));
        SerialWriteKV(f, ti, "EndColor", Vec4Str(pe->emitter.EndColor));
        SerialWriteKV(f, ti, "MinVelocity", Vec3Str(pe->emitter.MinVelocity));
        SerialWriteKV(f, ti, "MaxVelocity", Vec3Str(pe->emitter.MaxVelocity));
        SerialWriteKV(f, ti, "MaxParticles", std::to_string(pe->maxParticles));
    }

    if (auto* pp = reg.try_get<PostProcessComponent>(entity))
    {
        WriteComponentHeader(f, ci, "PostProcess");
        SerialWriteKV(f, ti, "Active", pp->enabled ? "true" : "false");
        std::string effects;
        for (auto& eff : pp->effects)
        {
            effects += eff.shaderName + ":" + std::to_string(eff.priority) + ":" + std::to_string(eff.x) + ":" +
                       std::to_string(eff.y) + ":" + std::to_string(eff.w) + ":" + std::to_string(eff.h) + ":" +
                       (eff.affectUI ? "1" : "0") + ":" + std::to_string(static_cast<uint32_t>(eff.inputs)) + ":" +
                       (eff.enabled ? "1" : "0") + " ";
        }
        if (!effects.empty())
            SerialWriteKV(f, ti, "Effects", effects);
    }

    // UI
    if (auto* uit = reg.try_get<UITransformComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UITransform");
        SerialWriteKV(f, ti, "position", Vec2PercentStr(uit->position, uit->positionIsPercent));
        SerialWriteKV(f, ti, "size", Vec2PercentStr(uit->size, uit->sizeIsPercent));
        SerialWriteKV(f, ti, "zIndex", std::to_string(uit->zIndex));
        SerialWriteKV(f, ti, "rotation", FloatStr(uit->rotation));
        SerialWriteKV(f, ti, "pivot", Vec2Str(uit->pivot));
        SerialWriteKV(f, ti, "flipX", uit->flipX ? "true" : "false");
        SerialWriteKV(f, ti, "flipY", uit->flipY ? "true" : "false");
        SerialWriteKV(f, ti, "anchorMin", Vec2PercentStr(uit->anchorMin, uit->anchorMinIsPercent));
        SerialWriteKV(f, ti, "anchorMax", Vec2PercentStr(uit->anchorMax, uit->anchorMaxIsPercent));
        SerialWriteKV(f, ti, "offsetMin", Vec2PercentStr(uit->offsetMin, uit->offsetMinIsPercent));
        SerialWriteKV(f, ti, "offsetMax", Vec2PercentStr(uit->offsetMax, uit->offsetMaxIsPercent));
    }

    if (auto* uir = reg.try_get<UIRendererComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIRenderer");
        SerialWriteKV(f, ti, "color", Vec4Str(uir->color));
        const std::string textureName = uir->texture ? uir->texture->path : uir->textureName;
        if (!textureName.empty())
            SerialWriteKV(f, ti, "texture", SceneSerializer::NormalizePath(textureName));
        if (!uir->shaderName.empty())
            SerialWriteKV(f, ti, "shader", uir->shaderName);
    }

    if (auto* uitext = reg.try_get<UITextComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIText");
        SerialWriteKV(f, ti, "text", "\"" + uitext->text + "\"");
        SerialWriteKV(f, ti, "color", Vec4Str(uitext->color));
        SerialWriteKV(f, ti, "scale", FloatStr(uitext->scale));
        SerialWriteKV(f, ti, "fontSize", std::to_string(uitext->font ? uitext->font->GetFontSize() : uitext->fontSize));
        SerialWriteKV(f, ti, "alignment",
                      (uitext->alignment == TextAlignment::Center
                           ? "Center"
                           : (uitext->alignment == TextAlignment::Right ? "Right" : "Left")));
        SerialWriteKV(f, ti, "wordWrap", uitext->wordWrap ? "true" : "false");
        SerialWriteKV(f, ti, "maxWidth", FloatStr(uitext->maxWidth));
        SerialWriteKV(f, ti, "wrapByWord", uitext->wrapByWord ? "true" : "false");
        if (!uitext->fontName.empty())
            SerialWriteKV(f, ti, "font", uitext->fontName);
        if (!uitext->shaderName.empty())
            SerialWriteKV(f, ti, "shader", uitext->shaderName);
    }

    if (auto* uif = reg.try_get<UIFlexLayoutComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIFlex");
        SerialWriteKV(f, ti, "direction", uif->direction == FlexDirection::Row ? "Row" : "Column");
        SerialWriteKV(f, ti, "spacing", FloatStr(uif->spacing));
        SerialWriteKV(f, ti, "autoSize", uif->autoSize ? "true" : "false");
        SerialWriteKV(f, ti, "padding", Vec4Str(uif->padding));
    }

    if (auto* interactive = reg.try_get<UIInteractiveComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIInteractive");
        SerialWriteKV(f, ti, "Interactable", interactive->interactable ? "true" : "false");
    }

    if (auto* animation = reg.try_get<UIAnimationComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIAnimation");
        SerialWriteKV(f, ti, "Enabled", animation->enabled ? "true" : "false");
        SerialWriteKV(f, ti, "AnimateColor", animation->animateColor ? "true" : "false");
        SerialWriteKV(f, ti, "AnimateScale", animation->animateScale ? "true" : "false");
        SerialWriteKV(f, ti, "NormalColor", Vec4Str(animation->normalColor));
        SerialWriteKV(f, ti, "HoverColor", Vec4Str(animation->hoverColor));
        SerialWriteKV(f, ti, "PressedColor", Vec4Str(animation->pressedColor));
        SerialWriteKV(f, ti, "NormalScale", FloatStr(animation->normalScale));
        SerialWriteKV(f, ti, "HoverScale", FloatStr(animation->hoverScale));
        SerialWriteKV(f, ti, "PressedScale", FloatStr(animation->pressedScale));
        SerialWriteKV(f, ti, "TransitionSpeed", FloatStr(animation->transitionSpeed));
    }

    if (auto* sky = reg.try_get<SkyboxRenderComponent>(entity))
    {
        WriteComponentHeader(f, ci, "SkyboxRenderer");
        const std::string skyboxName = sky->skybox ? sky->skybox->GetName() : sky->skyboxName;
        if (!skyboxName.empty())
            SerialWriteKV(f, ti, "Skybox", skyboxName);
        if (!sky->shaderName.empty())
            SerialWriteKV(f, ti, "Shader", sky->shaderName);
        SerialWriteKV(f, ti, "Primary", sky->isPrimary ? "true" : "false");
    }

    if (auto* rp = reg.try_get<ReflectionProbeComponent>(entity))
    {
        WriteComponentHeader(f, ci, "ReflectionProbe");
        SerialWriteKV(f, ti, "Type", rp->type == ReflectionProbeType::Dynamic ? "Dynamic" : "Static");
        SerialWriteKV(f, ti, "Resolution", std::to_string(rp->resolution));
        SerialWriteKV(f, ti, "BoxProjection", rp->boxProjection ? "true" : "false");
        SerialWriteKV(f, ti, "BoxMin", Vec3Str(rp->boxMin));
        SerialWriteKV(f, ti, "BoxMax", Vec3Str(rp->boxMax));
        SerialWriteKV(f, ti, "BlendDistance", FloatStr(rp->blendDistance));
    }
    if (auto* ref = reg.try_get<ReflectiveComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Reflective");
        SerialWriteKV(f, ti, "Active", ref->enabled ? "true" : "false");
        SerialWriteKV(f, ti, "Reflectivity", FloatStr(ref->reflectivity));
        SerialWriteKV(f, ti, "FresnelPower", FloatStr(ref->fresnelPower));
        SerialWriteKV(f, ti, "FresnelBias", FloatStr(ref->fresnelBias));
        if (!ref->targetProbe.empty())
            SerialWriteKV(f, ti, "Probe", ref->targetProbe);
    }
    if (auto* pr = reg.try_get<PlanarReflectionComponent>(entity))
    {
        WriteComponentHeader(f, ci, "PlanarReflection");
        SerialWriteKV(f, ti, "Resolution", std::to_string(pr->resolution));
        SerialWriteKV(f, ti, "ResolutionY", std::to_string(pr->resolution_y));
        SerialWriteKV(f, ti, "ResolutionScale", FloatStr(pr->resolutionScale));
        SerialWriteKV(f, ti, "UpdateIntervalFrames", std::to_string(pr->updateIntervalFrames));
        SerialWriteKV(f, ti, "Normal", Vec3Str(pr->normal));
    }

    if (auto* d = reg.try_get<DecalComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Decal");
        if (!d->albedoTexture.empty())
            SerialWriteKV(f, ti, "Albedo", d->albedoTexture);
        SerialWriteKV(f, ti, "Opacity", FloatStr(d->opacity));
        SerialWriteKV(f, ti, "Roughness", FloatStr(d->roughness));
        SerialWriteKV(f, ti, "Metallic", FloatStr(d->metallic));
        SerialWriteKV(f, ti, "Reflectivity", FloatStr(d->reflectivity));
        SerialWriteKV(f, ti, "TintColor", Vec4Str(d->tintColor));
        SerialWriteKV(f, ti, "Lifetime", FloatStr(d->lifetime));
        SerialWriteKV(f, ti, "RenderOrder", std::to_string(d->renderOrder));
        SerialWriteKV(f, ti, "LightingMode", std::to_string(d->lightingMode));
        if (!d->targetTags.empty())
        {
            std::ostringstream tags;
            for (size_t i = 0; i < d->targetTags.size(); ++i)
            {
                if (i != 0)
                    tags << ' ';
                tags << d->targetTags[i];
            }
            SerialWriteKV(f, ti, "TargetTags", tags.str());
        }
        if (!d->customShader.empty())
            SerialWriteKV(f, ti, "Shader", d->customShader);
    }

    if (auto* lp = reg.try_get<LightProbeComponent>(entity))
    {
        WriteComponentHeader(f, ci, "LightProbe");
        SerialWriteKV(f, ti, "Intensity", FloatStr(lp->intensity));
        SerialWriteKV(f, ti, "Radius", FloatStr(lp->radius));
        SerialWriteKV(f, ti, "Tint", Vec3Str(lp->tint));
        std::ostringstream sh;
        for (const glm::vec3& coefficient : lp->sh)
        {
            if (sh.tellp() > 0)
                sh << ' ';
            sh << Vec3Str(coefficient);
        }
        SerialWriteKV(f, ti, "SH", sh.str());
    }

    if (auto* t = reg.try_get<TerrainComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Terrain");
        if (!t->heightMapName.empty())
            SerialWriteKV(f, ti, "HeightMap", t->heightMapName);
        if (!t->splatMapName.empty())
            SerialWriteKV(f, ti, "SplatMap", t->splatMapName);
        auto writeNames = [&](const char* key, const std::vector<std::string>& names) {
            if (names.empty())
                return;
            std::ostringstream value;
            for (size_t i = 0; i < names.size(); ++i)
            {
                if (i != 0)
                    value << ' ';
                value << names[i];
            }
            SerialWriteKV(f, ti, key, value.str());
        };
        writeNames("DiffuseLayers", t->diffuseLayerNames);
        writeNames("NormalLayers", t->normalLayerNames);
        SerialWriteKV(f, ti, "Size", Vec3Str(t->terrainSize));
        SerialWriteKV(f, ti, "MaxHeight", FloatStr(t->maxHeight));
        SerialWriteKV(f, ti, "Resolution", std::to_string(t->resolution));
        SerialWriteKV(f, ti, "ChunkSize", std::to_string(t->chunkSize));
        SerialWriteKV(f, ti, "LODDistances", Vec3Str(t->lodDistances));
        SerialWriteKV(f, ti, "TextureScale", FloatStr(t->textureScale));
        SerialWriteKV(f, ti, "CastShadows", t->castShadows ? "true" : "false");
        SerialWriteKV(f, ti, "GeneratePhysics", t->generatePhysics ? "true" : "false");
        SerialWriteKV(f, ti, "Walkable", t->isWalkable ? "true" : "false");
        if (!t->customShader.empty())
            SerialWriteKV(f, ti, "Shader", t->customShader);
    }

    if (auto* lod = reg.try_get<LODComponent>(entity))
    {
        WriteComponentHeader(f, ci, "LOD");
        std::string models, dists;
        const size_t pairCount =
            std::min(std::max(lod->lodModels.size(), lod->lodModelNames.size()), lod->lodDistancesSq.size());
        for (size_t i = 0; i < pairCount; ++i)
        {
            const std::string modelName = i < lod->lodModels.size() && lod->lodModels[i]
                                              ? lod->lodModels[i]->GetName()
                                              : (i < lod->lodModelNames.size() ? lod->lodModelNames[i] : "");
            if (modelName.empty())
                continue;

            models += modelName + " ";
            dists += FloatStr(sqrtf(lod->lodDistancesSq[i])) + " ";
        }
        if (!models.empty())
            SerialWriteKV(f, ti, "Models", models);
        if (!dists.empty())
            SerialWriteKV(f, ti, "Distances", dists);
    }

    if (auto* follower = reg.try_get<PathFollowerComponent>(entity))
    {
        WriteComponentHeader(f, ci, "PathFollower");
        SerialWriteKV(f, ti, "MoveSpeed", FloatStr(follower->moveSpeed));
        SerialWriteKV(f, ti, "RotationSpeed", FloatStr(follower->rotationSpeed));
        SerialWriteKV(f, ti, "MaxRotationSpeed", FloatStr(follower->maxRotationSpeed));
        SerialWriteKV(f, ti, "RotationAcceleration", FloatStr(follower->rotationAcceleration));
        SerialWriteKV(f, ti, "RotationOffset", Vec3Str(follower->rotationOffset));
        SerialWriteKV(f, ti, "ArrivalDistance", FloatStr(follower->arrivalDistance));
        SerialWriteKV(f, ti, "RecordDebugPath", follower->recordDebugPath ? "true" : "false");
        SerialWriteKV(f, ti, "Provider", std::to_string(static_cast<int>(follower->pathfindingOptions.provider)));
        SerialWriteKV(f, ti, "Criteria", std::to_string(static_cast<int>(follower->pathfindingOptions.criteria)));
        std::ostringstream preferredTags;
        for (size_t i = 0; i < follower->pathfindingOptions.preferredTags.size(); ++i)
        {
            if (i != 0)
                preferredTags << ' ';
            preferredTags << follower->pathfindingOptions.preferredTags[i];
        }
        SerialWriteKV(f, ti, "PreferredTags", preferredTags.str());
        SerialWriteKV(f, ti, "TagWeightBonus", FloatStr(follower->pathfindingOptions.tagWeightBonus));
        SerialWriteKV(f, ti, "AltitudePenaltyWeight", FloatStr(follower->pathfindingOptions.altitudePenaltyWeight));
        std::string providerName = follower->navigationProviderName;
        if (reg.valid(follower->navigationProviderEntity))
        {
            if (auto* providerInfo = reg.try_get<InfoComponent>(follower->navigationProviderEntity))
                providerName = providerInfo->name;
        }
        if (!providerName.empty())
            SerialWriteKV(f, ti, "ProviderEntity", providerName);
        SerialWriteKV(f, ti, "LockXPitch", follower->lockXPitch ? "true" : "false");
        SerialWriteKV(f, ti, "LockYYaw", follower->lockYYaw ? "true" : "false");
        SerialWriteKV(f, ti, "LockZRoll", follower->lockZRoll ? "true" : "false");
        SerialWriteKV(f, ti, "LockMoveX", follower->lockMoveX ? "true" : "false");
        SerialWriteKV(f, ti, "LockMoveY", follower->lockMoveY ? "true" : "false");
        SerialWriteKV(f, ti, "LockMoveZ", follower->lockMoveZ ? "true" : "false");
        SerialWriteKV(f, ti, "LocalAvoidance", follower->localAvoidanceEnabled ? "true" : "false");
        SerialWriteKV(f, ti, "SeparationRadius", FloatStr(follower->separationRadius));
        SerialWriteKV(f, ti, "SeparationWeight", FloatStr(follower->separationWeight));
        SerialWriteKV(f, ti, "ObstacleAvoidanceDistance", FloatStr(follower->obstacleAvoidanceDistance));
        SerialWriteKV(f, ti, "ObstacleAvoidanceWeight", FloatStr(follower->obstacleAvoidanceWeight));
    }

    if (auto* nav = reg.try_get<NavMeshComponent>(entity))
    {
        WriteComponentHeader(f, ci, "NavMesh");
        SerialWriteKV(f, ti, "IsDynamic", nav->isDynamic ? "true" : "false");
        SerialWriteKV(f, ti, "NeedsRebuild", nav->needsRebuild ? "true" : "false");
        SerialWriteKV(f, ti, "TerrainGridResolution", std::to_string(nav->terrainGridResolution));
        SerialWriteKV(f, ti, "WalkableNormalY", FloatStr(nav->walkableNormalY));
        SerialWriteKV(f, ti, "CarveHeightPadding", FloatStr(nav->carveHeightPadding));
        SerialWriteKV(f, ti, "CarveAgentRadius", FloatStr(nav->carveAgentRadius));
        if (!nav->vertices.empty())
        {
            for (int i = 0; i < ti; ++i) f << "  ";
            f << "Vertices:\n";
            for (const glm::vec3& vertex : nav->vertices) SerialWriteKV(f, ti + 1, "Vertex", Vec3Str(vertex));
        }
        if (!nav->triangles.empty())
        {
            for (int i = 0; i < ti; ++i) f << "  ";
            f << "Triangles:\n";
            for (const NavMeshTriangle& triangle : nav->triangles)
            {
                for (int i = 0; i < ti + 1; ++i) f << "  ";
                f << "Triangle:\n";
                SerialWriteKV(f, ti + 2, "Indices", std::to_string(triangle.indices[0]) + " " +
                                                           std::to_string(triangle.indices[1]) + " " +
                                                           std::to_string(triangle.indices[2]));
                SerialWriteKV(f, ti + 2, "Center", Vec3Str(triangle.center));
                SerialWriteKV(f, ti + 2, "Normal", Vec3Str(triangle.normal));
                SerialWriteKV(f, ti + 2, "Tag", triangle.tag);
            }
        }
        if (!nav->nodes.empty())
        {
            for (int i = 0; i < ti; ++i) f << "  ";
            f << "Nodes:\n";
            for (const NavMeshNode& node : nav->nodes)
            {
                for (int i = 0; i < ti + 1; ++i) f << "  ";
                f << "Node:\n";
                SerialWriteKV(f, ti + 2, "Position", Vec3Str(node.position));
                SerialWriteKV(f, ti + 2, "TriangleIndex", std::to_string(node.triangleIndex));
                SerialWriteKV(f, ti + 2, "Tag", node.tag);
                std::ostringstream neighbors;
                for (size_t i = 0; i < node.neighbors.size(); ++i)
                {
                    if (i != 0)
                        neighbors << ' ';
                    neighbors << node.neighbors[i];
                }
                SerialWriteKV(f, ti + 2, "Neighbors", neighbors.str());
            }
        }
    }

    if (auto* grid = reg.try_get<NavigationGridComponent>(entity))
    {
        WriteComponentHeader(f, ci, "NavigationGrid");
        SerialWriteKV(f, ti, "Origin", Vec3Str(grid->origin));
        SerialWriteKV(f, ti, "Width", std::to_string(grid->width));
        SerialWriteKV(f, ti, "Height", std::to_string(grid->height));
        SerialWriteKV(f, ti, "CellSize", FloatStr(grid->cellSize));
        SerialWriteKV(f, ti, "AllowDiagonal", grid->allowDiagonal ? "true" : "false");
        if (!grid->cells.empty())
        {
            for (int i = 0; i < ti; ++i) f << "  ";
            f << "Cells:\n";
            for (const NavigationGridCell& cell : grid->cells)
            {
                for (int i = 0; i < ti + 1; ++i) f << "  ";
                f << "Cell:\n";
                SerialWriteKV(f, ti + 2, "Walkable", cell.walkable ? "true" : "false");
                SerialWriteKV(f, ti + 2, "Cost", FloatStr(cell.cost));
                SerialWriteKV(f, ti + 2, "Tag", cell.tag);
            }
        }
    }

    if (auto* occlusion = reg.try_get<OcclusionComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Occlusion");
        SerialWriteKV(f, ti, "Visible", occlusion->isVisible ? "true" : "false");
    }

    if (auto* streaming = reg.try_get<StreamingComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Streaming");
        SerialWriteKV(f, ti, "Model", SceneSerializer::NormalizePath(streaming->modelPath));
        SerialWriteKV(f, ti, "Static", streaming->isStatic ? "true" : "false");
        SerialWriteKV(f, ti, "LoadDistance", FloatStr(streaming->loadDistance));
        SerialWriteKV(f, ti, "UnloadDistance", FloatStr(streaming->unloadDistance));
    }

    if (auto* sc = reg.try_get<ScriptComponent>(entity); sc && !sc->className.empty())
    {
        WriteComponentHeader(f, ci, "Script");
        SerialWriteKV(f, ti, "Class", sc->className);
    }

    if (auto* net = reg.try_get<NetworkComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Network");
        SerialWriteKV(f, ti, "NetworkId", std::to_string(net->networkId));
        SerialWriteKV(f, ti, "OwnerId", std::to_string(net->ownerId));
        SerialWriteKV(f, ti, "IsLocal", net->isLocal ? "true" : "false");
        SerialWriteKV(f, ti, "ReplicateTransform", net->replicateTransform ? "true" : "false");
        SerialWriteKV(f, ti, "InterestRadius", FloatStr(net->interestRadius));
    }

    for (auto& [type, component] : ComponentLoader::CollectSerializedComponents(reg, entity))
    {
        if (IsBuiltInComponentType(type))
            continue;
        WriteComponentHeader(f, ci, type);
        YAMLWriter::Write(f, component.children, ti * YAMLWriter::IndentWidth);
    }

    // Parent
    if (auto* h = reg.try_get<HierarchyComponent>(entity))
    {
        if (h->parent != entt::null)
        {
            if (auto* pInfo = reg.try_get<InfoComponent>(h->parent))
            {
                if (!targetScene.empty() && SceneSerializer::NormalizeSceneName(pInfo->sceneName) != targetScene)
                {
                    SerialWriteKV(f, indent + 1, "Parent", pInfo->name);
                }
            }
        }
        for (auto child : h->children)
        {
            if (auto* cInfo = reg.try_get<InfoComponent>(child))
            {
                if (!cInfo->sceneName.empty() && !targetScene.empty() &&
                    SceneSerializer::NormalizeSceneName(cInfo->sceneName) != targetScene)
                    continue;
            }
            if (reg.all_of<FragmentComponent>(entity))
                continue;
            SerializeEntity(f, reg, child, indent + 1, sceneName);
        }
    }
}

static bool HasSerializableComponents(entt::registry& reg, entt::entity entity)
{
    auto* script = reg.try_get<ScriptComponent>(entity);
    const bool hasNamedScript = script && !script->className.empty();
    return hasNamedScript ||
           reg.any_of<PositionComponent, RotationComponent, ScaleComponent, MeshRendererComponent, MaterialComponent,
                      DirectionalLightComponent, PointLightComponent, SpotLightComponent, CameraComponent,
                      RigidShapeComponent, RigidBodyComponent, CharacterControllerComponent, AudioSourceComponent,
                      VideoPlayerComponent, AnimationComponent, ParticleEmitterComponent, PostProcessComponent,
                      UITransformComponent, UIRendererComponent, UITextComponent, UIFlexLayoutComponent,
                      UIInteractiveComponent, UIAnimationComponent, SkyboxRenderComponent, ReflectionProbeComponent,
                      ReflectiveComponent, PlanarReflectionComponent, DecalComponent, LightProbeComponent,
                       TerrainComponent, LODComponent, PathFollowerComponent, NavMeshComponent,
                       NavigationGridComponent, OcclusionComponent, StreamingComponent, NetworkComponent,
                       FragmentComponent>(entity) ||
           ComponentLoader::HasSerializedComponents(reg, entity);
}

bool SceneSerializer::Serialize(const std::string& filepath, const Scene& constScene, const std::string& sceneName)
{
    std::ofstream f(filepath);
    if (!f.is_open())
        return false;
    return SerializeToStream(f, constScene, sceneName);
}

std::string SceneSerializer::SerializeToString(const Scene& scene, const std::string& sceneName)
{
    std::ostringstream stream;
    if (!SerializeToStream(stream, scene, sceneName))
        return {};
    return stream.str();
}

bool SceneSerializer::SerializeToStream(std::ostream& f, const Scene& constScene, const std::string& sceneName)
{
    ResourceManager& res = m_Res;
    Scene& scene = const_cast<Scene&>(constScene);
    f << "axis_scene:\n";

    std::string normName = SceneSerializer::NormalizeSceneName(sceneName);
    auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>();
    const SceneRecord* rec = sceneMgr ? sceneMgr->GetSceneByName(normName) : nullptr;

    UsedResources ur;
    auto view = scene.View<InfoComponent>();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (info.isTransient)
            continue;
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName)
            continue;

        bool isRootInScene = true;
        if (auto* h = scene.TryGetComponent<HierarchyComponent>(entity))
        {
            if (h->parent != entt::null)
            {
                if (auto* pInfo = scene.TryGetComponent<InfoComponent>(h->parent))
                {
                    if (normName.empty() || SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName)
                        isRootInScene = false;
                }
            }
        }
        if (isRootInScene)
            CollectResources(scene.GetRegistry(), entity, ur, normName);
    }

    f << "  Resources:\n";
    auto IsOwnedByOther = [&](const std::string& name, const std::string& type) {
        if (!sceneMgr)
            return false;
        for (const auto& otherRec : sceneMgr->GetAllScenes())
        {
            if (SceneSerializer::NormalizeSceneName(otherRec.name) == normName)
                continue;

            const std::vector<std::string>* list = nullptr;
            if (type == "Shader")
                list = &otherRec.ownedShaders;
            else if (type == "Model")
                list = &otherRec.ownedModels;
            else if (type == "Texture")
                list = &otherRec.ownedTextures;
            else if (type == "Font")
                list = &otherRec.ownedFonts;
            else if (type == "Skybox")
                list = &otherRec.ownedSkyboxes;
            else if (type == "Animation")
                list = &otherRec.ownedAnimations;
            else if (type == "Audio" || type == "Sound")
                list = &otherRec.ownedSounds;

            if (list && std::find(list->begin(), list->end(), name) != list->end())
                return true;
        }
        return false;
    };

    std::map<std::string, std::set<std::string>> serializedPathsByType;

    for (const auto& def : res.GetResourceDefinitions())
    {
        bool used = false;
        if (def.type == "Shader")
            used = ur.shaders.count(def.name) || (rec && std::find(rec->ownedShaders.begin(), rec->ownedShaders.end(),
                                                                   def.name) != rec->ownedShaders.end());
        else if (def.type == "Model")
            used = ur.models.count(def.name) || (rec && std::find(rec->ownedModels.begin(), rec->ownedModels.end(),
                                                                  def.name) != rec->ownedModels.end());
        else if (def.type == "Texture")
        {
            std::string relPath =
                SceneSerializer::NormalizePath(def.properties.count("Path") ? def.properties.at("Path") : "");
            used = ur.textures.count(def.name) || ur.textures.count(relPath) ||
                   (rec && std::find(rec->ownedTextures.begin(), rec->ownedTextures.end(), def.name) !=
                               rec->ownedTextures.end());
        }
        else if (def.type == "Font")
            used = ur.fonts.count(def.name) || (rec && std::find(rec->ownedFonts.begin(), rec->ownedFonts.end(),
                                                                 def.name) != rec->ownedFonts.end());
        else if (def.type == "Skybox")
            used = ur.skyboxes.count(def.name) ||
                   (rec && std::find(rec->ownedSkyboxes.begin(), rec->ownedSkyboxes.end(), def.name) !=
                               rec->ownedSkyboxes.end());
        else if (def.type == "Animation")
            used = ur.animations.count(def.name) ||
                   (rec && std::find(rec->ownedAnimations.begin(), rec->ownedAnimations.end(), def.name) !=
                               rec->ownedAnimations.end());
        else if (def.type == "Audio" || def.type == "Sound")
            used = ur.sounds.count(def.name) || (rec && std::find(rec->ownedSounds.begin(), rec->ownedSounds.end(),
                                                                  def.name) != rec->ownedSounds.end());

        if (!used)
            continue;
        if (IsOwnedByOther(def.name, def.type))
            continue;

        // Deduplication by path
        if (def.properties.count("Path"))
        {
            std::string p = SceneSerializer::NormalizePath(def.properties.at("Path"));
            // For fonts, we need both path and size to uniquely identify
            if (def.type == "Font" && def.properties.count("Size"))
                p += ":" + def.properties.at("Size");

            if (serializedPathsByType[def.type].count(p))
            {
                continue;
            }
            serializedPathsByType[def.type].insert(p);
        }

        f << "    " << def.type << ":\n";
        f << "      Name: " << def.name << "\n";
        std::vector<std::string> propertyKeys;
        propertyKeys.reserve(def.properties.size());
        for (const auto& [key, value] : def.properties)
            propertyKeys.push_back(key);
        std::sort(propertyKeys.begin(), propertyKeys.end());
        for (const auto& propertyKey : propertyKeys)
        {
            std::string val = def.properties.at(propertyKey);
            if (propertyKey == "Path" || propertyKey == "Vertex" || propertyKey == "Fragment" ||
                propertyKey == "Geometry" || propertyKey == "Albedo" || propertyKey == "Normal" ||
                propertyKey == "MetallicMap" || propertyKey == "RoughnessMap" || propertyKey == "Right" ||
                propertyKey == "Left" || propertyKey == "Top" || propertyKey == "Bottom" ||
                propertyKey == "Front" || propertyKey == "Back")
            {
                val = SceneSerializer::NormalizePath(val);
            }
            SerialWriteKV(f, 3, propertyKey, val);
        }
    }

    f << "  Entities:\n";
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (info.isTransient)
            continue;
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName)
            continue;

        bool isRootInScene = true;
        if (auto* h = scene.TryGetComponent<HierarchyComponent>(entity))
        {
            if (h->parent != entt::null)
            {
                if (auto* pInfo = scene.TryGetComponent<InfoComponent>(h->parent))
                {
                    if (normName.empty() || SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName)
                        isRootInScene = false;
                }
            }
        }
        if (isRootInScene)
        {
            if (!HasSerializableComponents(scene.GetRegistry(), entity))
                continue;
            SerializeEntity(f, scene.GetRegistry(), entity, 2, normName);
        }
    }

    return f.good();
}
