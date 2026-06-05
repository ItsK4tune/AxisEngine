#include <scene/logic/scene_serializer.h>
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
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/physics_loader.h>
#include <platform/logic/monitor_manager.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_validator.h>
#include <algorithm>
#include <filesystem>
#include <set>

SceneSerializer::SceneSerializer(ResourceManager& res, IPhysicsWorld* phys, AudioService* audio)
    : m_Res(res), m_Phys(phys), m_Audio(audio)
{
}

bool SceneSerializer::Serialize(const std::string& filepath, const Scene& scene)
{
    return Serialize(filepath, const_cast<Scene&>(scene), m_Res, "");
}

bool SceneSerializer::Deserialize(const std::string& filepath, Scene& scene)
{
    SceneLoadResult res = Deserialize(filepath, scene, m_Res, m_Phys, m_Audio);
    return !res.entities.empty() || res.hasConfig;
}

SceneLoadResult SceneSerializer::Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res,
                                             IPhysicsWorld* phys, AudioService* sound)
{
    SceneLoadResult result;
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
        return result;
    }

    std::string sceneName = filepath;
    size_t slash = sceneName.find_last_of("/\\");
    if (slash != std::string::npos)
        sceneName = sceneName.substr(slash + 1);
    auto dotPos = sceneName.rfind('.');
    if (dotPos != std::string::npos)
        sceneName = sceneName.substr(0, dotPos);

    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    std::vector<YAMLNode> activeRoots = roots;
    if (roots.size() == 1 && roots[0].key == "axis_scene")
    {
        activeRoots = roots[0].children;
    }

    for (auto& root : activeRoots)
    {
        if (root.key == "Config")
        {
            auto& sl = ServiceLocator::Instance();
            auto& configMgr = sl.Require<ConfigManager>();
            AppConfig tempConfig = configMgr.GetConfig();
            for (auto& cfgNode : root.children)
            {
                std::stringstream ss;
                ss << cfgNode.key << " " << cfgNode.value;
                ConfigLoader::LoadConfig(ss, tempConfig, configMgr.IsHeadless());
            }
            configMgr.UpdateConfig(tempConfig);
            result.hasConfig = true;
            result.appliedConfig = tempConfig;
        }
        else if (root.key == "Resources")
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
                    unsigned int size = std::stoul(resNode.GetChildValue("Size", "16"));
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
                        res.LoadSound(name, path, sound->GetEngine());
                        result.loadedSounds.push_back(name);
                    }
                }
            }
        }
        else if (root.key == "Entities")
        {
            ComponentLoader::InitializeDefaultLoaders();
            std::function<void(YAMLNode&, entt::entity)> ParseEntity = [&](YAMLNode& entNode, entt::entity parent) {
                std::string entityName = entNode.key;
                entt::entity currentEntity = scene.registry.create();
                scene.registry.emplace<PositionComponent>(currentEntity);
                scene.registry.emplace<RotationComponent>(currentEntity);
                scene.registry.emplace<ScaleComponent>(currentEntity);
                scene.registry.emplace<HierarchyComponent>(currentEntity);
                scene.registry.emplace<WorldTransformComponent>(currentEntity);
                scene.registry.emplace<InfoComponent>(currentEntity, entityName,
                                                      entNode.GetChildValue("Tag", "default"));

                auto& info = scene.registry.get<InfoComponent>(currentEntity);
                info.sceneName = entNode.GetChildValue("Scene", entNode.GetChildValue("SceneName", sceneName));
                result.entities.push_back(currentEntity);

                if (parent != entt::null)
                {
                    auto& h = scene.registry.get<HierarchyComponent>(currentEntity);
                    h.parent = parent;
                    scene.registry.get<HierarchyComponent>(parent).children.push_back(currentEntity);
                }
                else if (auto* pNode = entNode.GetChild("Parent"))
                {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto& child : entNode.children)
                {
                    if (child.key == "Component")
                    {
                        ComponentLoader::Load(child.value, scene, currentEntity, child, res, phys);
                    }
                    else if (child.key != "Tag" && child.key != "Layer" && child.key != "Parent" &&
                             child.key != "Scene" && child.key != "SceneName")
                    {
                        ParseEntity(child, currentEntity);
                    }
                }
            };

            for (auto& entNode : root.children) ParseEntity(entNode, entt::null);
        }
    }

    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);
    SceneHandlers::SceneValidator::ValidateCamera(scene);

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS file: " << fullPath;
    return result;
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
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
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

static void WriteComponentHeader(std::ofstream& f, int indent, const std::string& type)
{
    for (int i = 0; i < indent; ++i) f << "  ";
    f << "Component: " << type << "\n";
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
        if (mr->model)
            ur.models.insert(mr->model->GetName());
    }
    if (auto* mat = reg.try_get<AxisMaterialComponent>(entity))
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
        if (!sky->shaderName.empty())
            ur.shaders.insert(sky->shaderName);
    }
    if (auto* uir = reg.try_get<UIRendererComponent>(entity))
    {
        if (uir->texture)
            ur.textures.insert(SceneSerializer::NormalizePath(uir->texture->path));
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

    // Recursively visit children
    if (auto* h = reg.try_get<HierarchyComponent>(entity))
    {
        for (auto child : h->children)
        {
            CollectResources(reg, child, ur, sceneName);
        }
    }
}

static void SerializeEntity(std::ofstream& f, entt::registry& reg, entt::entity entity, int indent,
                            const std::string& sceneName)
{
    auto* info = reg.try_get<InfoComponent>(entity);
    std::string name = info ? info->name : ("Entity_" + std::to_string((uint32_t)entity));
    std::string targetScene = SceneSerializer::NormalizeSceneName(sceneName);

    if (info && !targetScene.empty() && SceneSerializer::NormalizeSceneName(info->sceneName) != targetScene)
        return;

    for (int i = 0; i < indent; ++i) f << "  ";
    f << name << ":\n";
    int ci = indent + 1, ti = ci + 1;

    if (info && !info->tag.empty() && info->tag != "default")
        SerialWriteKV(f, ci, "Tag", info->tag);
    if (info && !info->isActive)
        SerialWriteKV(f, ci, "Active", "false");
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

    if (auto* mr = reg.try_get<MeshRendererComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Renderer");
        SerialWriteKV(f, ti, "Model", mr->model ? mr->model->GetName() : "");
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

    if (auto* mat = reg.try_get<AxisMaterialComponent>(entity))
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
    }

    // Lights
    if (auto* l = reg.try_get<DirectionalLightComponent>(entity))
    {
        WriteComponentHeader(f, ci, "DirectionalLight");
        SerialWriteKV(f, ti, "Active", l->active ? "true" : "false");
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
        SerialWriteKV(f, ti, "Color", Vec3Str(l->color));
        SerialWriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        SerialWriteKV(f, ti, "Radius", FloatStr(l->radius));
    }
    if (auto* l = reg.try_get<SpotLightComponent>(entity))
    {
        WriteComponentHeader(f, ci, "SpotLight");
        SerialWriteKV(f, ti, "Active", l->active ? "true" : "false");
        SerialWriteKV(f, ti, "CastShadow", l->isCastShadow ? "true" : "false");
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
    }
    if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
    {
        WriteComponentHeader(f, ci, "RigidBody");
        SerialWriteKV(f, ti, "Mass", FloatStr(rb->mass));
        SerialWriteKV(f, ti, "BodyType", rb->isStatic ? "STATIC" : (rb->isKinematic ? "KINEMATIC" : "DYNAMIC"));
        SerialWriteKV(f, ti, "LinearDamping", FloatStr(rb->linearDamping));
        SerialWriteKV(f, ti, "AngularDamping", FloatStr(rb->angularDamping));
        const glm::vec3 linearVelocity = rb->body ? rb->body->GetLinearVelocity() : rb->initialLinearVelocity;
        const glm::vec3 angularVelocity = rb->body ? rb->body->GetAngularVelocity() : rb->initialAngularVelocity;
        if (glm::length(linearVelocity) > 0.0001f)
            SerialWriteKV(f, ti, "LinearVelocity", Vec3Str(linearVelocity));
        if (glm::length(angularVelocity) > 0.0001f)
            SerialWriteKV(f, ti, "AngularVelocity", Vec3Str(angularVelocity));
        if (rb->isTrigger)
            SerialWriteKV(f, ti, "IsTrigger", "true");
    }

    if (auto* frag = reg.try_get<FragmentComponent>(entity))
    {
        LOGGER_INFO("SceneSerializer") << "[FRAG-SAVE] Entity '" << name << "' has Fragment. path='" << frag->path
                                       << "' overrides.size=" << frag->overrides.size() << " overrides='"
                                       << frag->overrides << "'";
        WriteComponentHeader(f, ci, "Fragment");
        SerialWriteKV(f, ti, "Path", SceneSerializer::NormalizePath(frag->path));
        if (!frag->overrides.empty())
        {
            // Parse the override string back into nodes for structural serialization
            auto overrideRoots = YAMLParser::ParseString(frag->overrides);
            if (!overrideRoots.empty())
            {
                f << std::string(ti * 2, ' ') << "Overrides:\n";
                std::function<void(const YAMLNode&, int)> writeOverrideNode = [&](const YAMLNode& n, int depth) {
                    f << std::string(depth * 2, ' ') << n.key;
                    if (!n.value.empty())
                        f << ": " << n.value;
                    f << (n.value.empty() ? ":" : "") << "\n";
                    for (const auto& child : n.children)
                    {
                        writeOverrideNode(child, depth + 1);
                    }
                };
                for (const auto& root : overrideRoots)
                {
                    writeOverrideNode(root, ti + 1);
                }
            }
        }
    }

    if (auto* audio = reg.try_get<AudioSourceComponent>(entity))
    {
        WriteComponentHeader(f, ci, "AudioSource");
        if (!audio->resourceName.empty())
            SerialWriteKV(f, ti, "Audio", audio->resourceName);
        SerialWriteKV(f, ti, "PlayOnAwake", audio->playOnAwake ? "true" : "false");
        SerialWriteKV(f, ti, "Loop", audio->loop ? "true" : "false");
        SerialWriteKV(f, ti, "Volume", FloatStr(audio->volume));
        SerialWriteKV(f, ti, "Pitch", FloatStr(audio->pitch));
        SerialWriteKV(f, ti, "Speed", FloatStr(audio->speed));
        SerialWriteKV(f, ti, "Is3d", audio->is3D ? "true" : "false");
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
        SerialWriteKV(f, ti, "StartSize", FloatStr(pe->emitter.StartSize));
        SerialWriteKV(f, ti, "EndSize", FloatStr(pe->emitter.EndSize));
        SerialWriteKV(f, ti, "StartColor", Vec4Str(pe->emitter.StartColor));
        SerialWriteKV(f, ti, "EndColor", Vec4Str(pe->emitter.EndColor));
        SerialWriteKV(f, ti, "MinVelocity", Vec3Str(pe->emitter.MinVelocity));
        SerialWriteKV(f, ti, "MaxVelocity", Vec3Str(pe->emitter.MaxVelocity));
    }

    if (auto* pp = reg.try_get<PostProcessComponent>(entity))
    {
        WriteComponentHeader(f, ci, "PostProcess");
        SerialWriteKV(f, ti, "Active", pp->enabled ? "true" : "false");
        std::string effects;
        for (auto& eff : pp->effects)
        {
            effects += eff.shaderName + ":" + std::to_string(eff.priority) + ":" + FloatStr(eff.x) + ":" +
                       FloatStr(eff.y) + ":" + FloatStr(eff.w) + ":" + FloatStr(eff.h) + ":" +
                       (eff.affectUI ? "1" : "0") + " ";
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
        if (uir->texture)
            SerialWriteKV(f, ti, "texture", SceneSerializer::NormalizePath(uir->texture->path));
        if (!uir->shaderName.empty())
            SerialWriteKV(f, ti, "shader", uir->shaderName);
    }

    if (auto* uitext = reg.try_get<UITextComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIText");
        SerialWriteKV(f, ti, "text", "\"" + uitext->text + "\"");
        SerialWriteKV(f, ti, "color", Vec4Str(uitext->color));
        SerialWriteKV(f, ti, "scale", FloatStr(uitext->scale));
        if (uitext->font)
            SerialWriteKV(f, ti, "fontSize", std::to_string(uitext->font->GetFontSize()));
        SerialWriteKV(f, ti, "alignment",
                      (uitext->alignment == TextAlignment::Center
                           ? "Center"
                           : (uitext->alignment == TextAlignment::Right ? "Right" : "Left")));
        SerialWriteKV(f, ti, "wordWrap", uitext->wordWrap ? "true" : "false");
        SerialWriteKV(f, ti, "maxWidth", FloatStr(uitext->maxWidth));
        if (!uitext->fontName.empty())
            SerialWriteKV(f, ti, "font", uitext->fontName);
    }

    if (auto* uif = reg.try_get<UIFlexLayoutComponent>(entity))
    {
        WriteComponentHeader(f, ci, "UIFlex");
        SerialWriteKV(f, ti, "direction", uif->direction == FlexDirection::Row ? "Row" : "Column");
        SerialWriteKV(f, ti, "spacing", FloatStr(uif->spacing));
        SerialWriteKV(f, ti, "autoSize", uif->autoSize ? "true" : "false");
        SerialWriteKV(f, ti, "padding", Vec4Str(uif->padding));
    }

    if (auto* sky = reg.try_get<SkyboxRenderComponent>(entity))
    {
        WriteComponentHeader(f, ci, "SkyboxRenderer");
        if (sky->skybox)
            SerialWriteKV(f, ti, "Skybox", sky->skybox->GetName());
        if (!sky->shaderName.empty())
            SerialWriteKV(f, ti, "Shader", sky->shaderName);
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
        SerialWriteKV(f, ti, "Normal", Vec3Str(pr->normal));
    }

    if (auto* d = reg.try_get<DecalComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Decal");
        SerialWriteKV(f, ti, "Opacity", FloatStr(d->opacity));
        SerialWriteKV(f, ti, "Roughness", FloatStr(d->roughness));
        SerialWriteKV(f, ti, "Metallic", FloatStr(d->metallic));
        SerialWriteKV(f, ti, "Reflectivity", FloatStr(d->reflectivity));
        SerialWriteKV(f, ti, "TintColor", Vec4Str(d->tintColor));
        if (!d->customShader.empty())
            SerialWriteKV(f, ti, "Shader", d->customShader);
    }

    if (auto* lp = reg.try_get<LightProbeComponent>(entity))
    {
        WriteComponentHeader(f, ci, "LightProbe");
        SerialWriteKV(f, ti, "Intensity", FloatStr(lp->intensity));
        SerialWriteKV(f, ti, "Radius", FloatStr(lp->radius));
    }

    if (auto* t = reg.try_get<TerrainComponent>(entity))
    {
        WriteComponentHeader(f, ci, "Terrain");
        if (!t->heightMapName.empty())
            SerialWriteKV(f, ti, "HeightMap", t->heightMapName);
        SerialWriteKV(f, ti, "Size", Vec3Str(t->terrainSize));
        SerialWriteKV(f, ti, "Resolution", std::to_string(t->resolution));
        SerialWriteKV(f, ti, "TextureScale", FloatStr(t->textureScale));
        SerialWriteKV(f, ti, "CastShadows", t->castShadows ? "true" : "false");
        if (!t->customShader.empty())
            SerialWriteKV(f, ti, "Shader", t->customShader);
    }

    if (auto* lod = reg.try_get<LODComponent>(entity))
    {
        WriteComponentHeader(f, ci, "LOD");
        std::string models, dists;
        const size_t pairCount = std::min(lod->lodModels.size(), lod->lodDistancesSq.size());
        for (size_t i = 0; i < pairCount; ++i)
        {
            if (!lod->lodModels[i])
                continue;

            models += lod->lodModels[i]->GetName() + " ";
            dists += FloatStr(sqrtf(lod->lodDistancesSq[i])) + " ";
        }
        if (!models.empty())
            SerialWriteKV(f, ti, "Models", models);
        if (!dists.empty())
            SerialWriteKV(f, ti, "Distances", dists);
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
           reg.any_of<PositionComponent, RotationComponent, ScaleComponent, MeshRendererComponent,
                      AxisMaterialComponent, DirectionalLightComponent, PointLightComponent, SpotLightComponent,
                      CameraComponent, RigidShapeComponent, RigidBodyComponent, CharacterControllerComponent,
                      AudioSourceComponent, VideoPlayerComponent, AnimationComponent, ParticleEmitterComponent,
                      PostProcessComponent, UITransformComponent, UIRendererComponent, UITextComponent,
                      UIFlexLayoutComponent, SkyboxRenderComponent, ReflectionProbeComponent, ReflectiveComponent,
                      PlanarReflectionComponent, DecalComponent, LightProbeComponent, TerrainComponent, LODComponent,
                      NetworkComponent, FragmentComponent>(entity);
}

bool SceneSerializer::Serialize(const std::string& filepath, Scene& scene, ResourceManager& res,
                                const std::string& sceneName)
{
    std::ofstream f(filepath);
    if (!f.is_open())
        return false;
    f << "axis_scene:\n";

    std::string normName = SceneSerializer::NormalizeSceneName(sceneName);
    auto& sl = ServiceLocator::Instance();
    auto& configMgr = sl.Require<ConfigManager>();
    auto* sceneMgr = sl.Resolve<SceneManager>();
    const SceneRecord* rec = sceneMgr ? sceneMgr->GetSceneByName(normName) : nullptr;

    if (rec && rec->hasConfig)
    {
        AppConfig cfg = configMgr.GetConfig();
        f << "  Config:\n";
        SerialWriteKV(f, 2, "WINDOW_WIDTH", std::to_string(cfg.window.width));
        SerialWriteKV(f, 2, "WINDOW_HEIGHT", std::to_string(cfg.window.height));

        auto WindowModeToStr = [](WindowMode mode) {
            switch (mode)
            {
                case WindowMode::Fullscreen:
                    return "FULLSCREEN";
                case WindowMode::Borderless:
                    return "BORDERLESS";
                case WindowMode::BorderlessFullscreen:
                    return "BORDERLESS_FULLSCREEN";
                default:
                    return "WINDOWED";
            }
        };
        SerialWriteKV(f, 2, "WINDOW_MODE", WindowModeToStr(cfg.window.windowMode));
        SerialWriteKV(f, 2, "VSYNC", cfg.window.vsync ? "1" : "0");

        SerialWriteKV(f, 2, "MSAA", std::to_string(cfg.graphics.msaaSamples));
        SerialWriteKV(f, 2, "RENDER_SCALE", FloatStr(cfg.graphics.renderScale));
        SerialWriteKV(f, 2, "ASYNC_RESOURCES", cfg.graphics.asyncResourceLoading ? "1" : "0");
        SerialWriteKV(f, 2, "STRICT_ASSET_LOADING", cfg.graphics.strictAssetLoading ? "1" : "0");

        SerialWriteKV(f, 2, "HDR_ENABLED", cfg.render.hdrEnabled ? "1" : "0");
        SerialWriteKV(f, 2, "BLOOM_ENABLED", cfg.render.bloomEnabled ? "1" : "0");
        SerialWriteKV(f, 2, "GAMMA", FloatStr(cfg.render.gamma));
        SerialWriteKV(f, 2, "EXPOSURE", FloatStr(cfg.render.exposure));
        SerialWriteKV(f, 2, "SKYBOX_INTENSITY", FloatStr(cfg.render.skyboxIntensity));
        SerialWriteKV(f, 2, "AMBIENT_INTENSITY", FloatStr(cfg.render.ambientIntensity));
        SerialWriteKV(f, 2, "UI_REFERENCE_SIZE",
                      Vec2Str(glm::vec2(cfg.render.uiReferenceWidth, cfg.render.uiReferenceHeight)));

        auto TonemappingToStr = [](TonemappingMode mode) {
            switch (mode)
            {
                case TonemappingMode::ACES:
                    return "ACES";
                case TonemappingMode::Reinhard:
                    return "REINHARD";
                default:
                    return "NONE";
            }
        };
        SerialWriteKV(f, 2, "TONEMAPPING", TonemappingToStr(cfg.render.tonemappingMode));
        SerialWriteKV(f, 2, "SHADOWS_ENABLED", cfg.shadow.shadowsEnabled ? "1" : "0");
        SerialWriteKV(f, 2, "SHADOW_RESOLUTION", std::to_string(cfg.shadow.shadowMapResolution));
        SerialWriteKV(f, 2, "SHADOW_BIAS", FloatStr(cfg.shadow.shadowBias));
        SerialWriteKV(f, 2, "SHADOW_SOFTNESS", std::to_string(cfg.shadow.shadowSoftness));

        SerialWriteKV(f, 2, "VOLUME", FloatStr(cfg.audio.masterVolume));

        SerialWriteKV(f, 2, "GRAVITY",
                      Vec3Str(glm::vec3(cfg.physics.gravity[0], cfg.physics.gravity[1], cfg.physics.gravity[2])));
        SerialWriteKV(f, 2, "MAX_SUBSTEPS", std::to_string(cfg.physics.maxSubSteps));
        SerialWriteKV(f, 2, "PHYSICS_TICKRATE", FloatStr(cfg.physics.physicsTickRate));

        auto LightingModeToStr = [](LightingMode mode) {
            switch (mode)
            {
                case LightingMode::Bake:
                    return "BAKE";
                case LightingMode::LightProbe:
                    return "LIGHT_PROBE";
                case LightingMode::ReflectionProbes:
                    return "REFLECTION_PROBES";
                default:
                    return "REAL_TIME";
            }
        };
        SerialWriteKV(f, 2, "LIGHTING_MODE", LightingModeToStr(cfg.lightingMode));
    }

    UsedResources ur;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName)
            continue;

        bool isRootInScene = true;
        if (auto* h = scene.registry.try_get<HierarchyComponent>(entity))
        {
            if (h->parent != entt::null)
            {
                if (auto* pInfo = scene.registry.try_get<InfoComponent>(h->parent))
                {
                    if (normName.empty() || SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName)
                        isRootInScene = false;
                }
            }
        }
        if (isRootInScene)
            CollectResources(scene.registry, entity, ur, normName);
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
        for (const auto& prop : def.properties)
        {
            std::string val = prop.second;
            if (prop.first == "Path" || prop.first == "Vertex" || prop.first == "Fragment" ||
                prop.first == "Geometry" || prop.first == "Albedo" || prop.first == "Normal" ||
                prop.first == "MetallicMap" || prop.first == "RoughnessMap" || prop.first == "Right" ||
                prop.first == "Left" || prop.first == "Top" || prop.first == "Bottom" || prop.first == "Front" ||
                prop.first == "Back")
            {
                val = SceneSerializer::NormalizePath(val);
            }
            SerialWriteKV(f, 3, prop.first, val);
        }
    }

    f << "  Entities:\n";
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName)
            continue;

        bool isRootInScene = true;
        if (auto* h = scene.registry.try_get<HierarchyComponent>(entity))
        {
            if (h->parent != entt::null)
            {
                if (auto* pInfo = scene.registry.try_get<InfoComponent>(h->parent))
                {
                    if (normName.empty() || SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName)
                        isRootInScene = false;
                }
            }
        }
        if (isRootInScene)
        {
            if (!HasSerializableComponents(scene.registry, entity))
                continue;
            if (scene.registry.all_of<FragmentComponent>(entity))
            {
                auto& fc = scene.registry.get<FragmentComponent>(entity);
                LOGGER_INFO("SceneSerializer") << "[FRAG-ROOT] Writing root fragment entity '" << info.name
                                               << "' overrides.size=" << fc.overrides.size();
            }
            SerializeEntity(f, scene.registry, entity, 2, normName);
        }
    }

    return true;
}
