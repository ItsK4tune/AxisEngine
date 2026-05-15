#include <filesystem>
#include <algorithm>
#include <set>
#include <ecs/unit/core_components.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <core/logic/config_loader.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/unit/script_component.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/physics_loader.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_validator.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/fragment_component.h>
#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/loader_utils.h>
#include <audio/logic/audio_service.h>
#include <audio/interface/i_audio_engine.h>

SceneLoadResult SceneSerializer::Deserialize(const std::string &filepath, Scene &scene, ResourceManager &res, IPhysicsWorld* phys, AudioService* sound)
{
    SceneLoadResult result;
    std::string fullPath = FileSystem::getPath(filepath);
    auto roots = YAMLParser::Parse(fullPath);
    if (roots.empty())
    {
        if (!std::filesystem::exists(fullPath)) {
            LOGGER_ERROR("SceneSerializer") << "AXS file does not exist: " << fullPath;
        } else {
            LOGGER_ERROR("SceneSerializer") << "Failed to parse AXS file (empty or malformed): " << fullPath;
        }
        return result;
    }

    std::string sceneName = filepath;
    size_t slash = sceneName.find_last_of("/\\");
    if (slash != std::string::npos) sceneName = sceneName.substr(slash + 1);
    auto dotPos = sceneName.rfind('.');
    if (dotPos != std::string::npos) sceneName = sceneName.substr(0, dotPos);

    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    std::vector<YAMLNode> activeRoots = roots;
    if (roots.size() == 1 && roots[0].key == "axis_scene")
    {
        activeRoots = roots[0].children;
    }

    for (auto &root : activeRoots)
    {
        if (root.key == "Config")
        {
            auto& sl = ServiceLocator::Instance();
            auto& configMgr = sl.Require<ConfigManager>();
            AppConfig tempConfig = configMgr.GetConfig();
            for (auto &cfgNode : root.children)
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
            for (auto &resNode : root.children)
            {
                if (resNode.key == "Shader")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string vs = resNode.GetChildValue("VS", resNode.GetChildValue("vertex", resNode.GetChildValue("Vertex")));
                    std::string fs = resNode.GetChildValue("FS", resNode.GetChildValue("fragment", resNode.GetChildValue("Fragment")));
                    std::string gs = resNode.GetChildValue("GS", resNode.GetChildValue("geometry", resNode.GetChildValue("Geometry")));
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
                    std::vector<std::string> faces = {
                        resNode.GetChildValue("Right"), resNode.GetChildValue("Left"),
                        resNode.GetChildValue("Top"), resNode.GetChildValue("Bottom"),
                        resNode.GetChildValue("Front"), resNode.GetChildValue("Back")
                    };
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
                    if (sound) {
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
                scene.registry.emplace<InfoComponent>(currentEntity, entityName, entNode.GetChildValue("Tag", "default"));
                
                auto& info = scene.registry.get<InfoComponent>(currentEntity);
                info.sceneName = sceneName;
                result.entities.push_back(currentEntity);

                if (parent != entt::null) {
                    auto& h = scene.registry.get<HierarchyComponent>(currentEntity);
                    h.parent = parent;
                    scene.registry.get<HierarchyComponent>(parent).children.push_back(currentEntity);
                } else if (auto* pNode = entNode.GetChild("Parent")) {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto& child : entNode.children) {
                    if (child.key == "Component") {
                        ComponentLoader::Load(child.value, scene, currentEntity, child, res, phys);
                    } else if (child.key != "Tag" && child.key != "Layer" && child.key != "Parent") {
                        ParseEntity(child, currentEntity);
                    }
                }
            };

            for (auto &entNode : root.children) ParseEntity(entNode, entt::null);
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

#include <fstream>
#include <sstream>
#include <ecs/unit/render_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/ui_components.h>

std::string SceneSerializer::NormalizeSceneName(const std::string& name) {
    std::string n = name;
    size_t slash = n.find_last_of("/\\");
    if (slash != std::string::npos) n = n.substr(slash + 1);
    size_t dot = n.rfind('.');
    if (dot != std::string::npos) n = n.substr(0, dot);
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    return n;
}

std::string SceneSerializer::NormalizePath(const std::string& path) {
    if (path.empty()) return "";
    return FileSystem::getRelativePath(path);
}

static std::string FloatStr(float f) {
    std::ostringstream ss; ss << std::fixed << std::setprecision(6) << f; return ss.str();
}

static std::string Vec2Str(const glm::vec2& v) {
    std::ostringstream ss; ss << FloatStr(v.x) << " " << FloatStr(v.y); return ss.str();
}

static std::string Vec3Str(const glm::vec3& v) {
    std::ostringstream ss; ss << FloatStr(v.x) << " " << FloatStr(v.y) << " " << FloatStr(v.z); return ss.str();
}

static std::string Vec4Str(const glm::vec4& v) {
    std::ostringstream ss; ss << FloatStr(v.x) << " " << FloatStr(v.y) << " " << FloatStr(v.z) << " " << FloatStr(v.w); return ss.str();
}

static void WriteKV(std::ofstream& f, int indent, const std::string& key, const std::string& val) {
    for (int i = 0; i < indent; ++i) f << "  ";
    f << key << ": " << val << "\n";
}

static std::string Vec2PercentStr(const glm::vec2& v, const glm::bvec2& p) {
    std::ostringstream ss;
    ss << FloatStr(v.x); if (p.x) ss << "%";
    ss << " " << FloatStr(v.y); if (p.y) ss << "%";
    return ss.str();
}

static void WriteComponentHeader(std::ofstream& f, int indent, const std::string& type) {
    for (int i = 0; i < indent; ++i) f << "  ";
    f << "Component: " << type << "\n";
}

struct UsedResources {
    std::set<std::string> shaders;
    std::set<std::string> models;
    std::set<std::string> textures;
    std::set<std::string> fonts;
    std::set<std::string> skyboxes;
    std::set<std::string> animations;
    std::set<std::string> sounds;
};

static void CollectResources(entt::registry& reg, entt::entity entity, UsedResources& ur, const std::string& sceneName) {
    if (auto* info = reg.try_get<InfoComponent>(entity)) {
        if (!sceneName.empty() && SceneSerializer::NormalizeSceneName(info->sceneName) != SceneSerializer::NormalizeSceneName(sceneName)) return;
    }

    if (auto* mr = reg.try_get<MeshRendererComponent>(entity)) {
        if (!mr->shaderName.empty()) ur.shaders.insert(mr->shaderName);
        if (mr->model) ur.models.insert(mr->model->GetName());
    }
    if (auto* mat = reg.try_get<AxisMaterialComponent>(entity)) {
        if (!mat->desc.albedoPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.albedoPath));
        if (!mat->desc.normalPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.normalPath));
        if (!mat->desc.metallicPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.metallicPath));
        if (!mat->desc.roughnessPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.roughnessPath));
        if (!mat->desc.aoPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.aoPath));
        if (!mat->desc.emissivePath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.emissivePath));
        if (!mat->desc.specularPath.empty()) ur.textures.insert(SceneSerializer::NormalizePath(mat->desc.specularPath));
    }
    if (auto* anim = reg.try_get<AnimationComponent>(entity)) {
        for (auto& a : anim->animations) ur.animations.insert(a);
    }
    if (auto* pe = reg.try_get<ParticleEmitterComponent>(entity)) {
        if (!pe->textureName.empty()) ur.textures.insert(pe->textureName);
        if (!pe->customShader.empty()) ur.shaders.insert(pe->customShader);
    }
    if (auto* sky = reg.try_get<SkyboxRenderComponent>(entity)) {
        if (sky->skybox) ur.skyboxes.insert(sky->skybox->GetName());
        if (!sky->shaderName.empty()) ur.shaders.insert(sky->shaderName);
    }
    if (auto* uir = reg.try_get<UIRendererComponent>(entity)) {
        if (uir->texture) ur.textures.insert(SceneSerializer::NormalizePath(uir->texture->path));
        if (!uir->shaderName.empty()) ur.shaders.insert(uir->shaderName);
    }
    if (auto* uit = reg.try_get<UITextComponent>(entity)) {
        if (!uit->fontName.empty()) ur.fonts.insert(uit->fontName);
        if (!uit->shaderName.empty()) ur.shaders.insert(uit->shaderName);
    }
    if (auto* pp = reg.try_get<PostProcessComponent>(entity)) {
        for (auto& eff : pp->effects) ur.shaders.insert(eff.shaderName);
    }
    if (auto* audio = reg.try_get<AudioSourceComponent>(entity)) {
        if (!audio->resourceName.empty()) ur.sounds.insert(audio->resourceName);
    }

    // Recursively visit children
    if (auto* h = reg.try_get<HierarchyComponent>(entity)) {
        for (auto child : h->children) {
            CollectResources(reg, child, ur, sceneName);
        }
    }
}

static void SerializeEntity(std::ofstream& f, entt::registry& reg, entt::entity entity, int indent, const std::string& sceneName) {
    auto* info = reg.try_get<InfoComponent>(entity);
    std::string name = info ? info->name : ("Entity_" + std::to_string((uint32_t)entity));
    std::string targetScene = SceneSerializer::NormalizeSceneName(sceneName);

    if (info && !targetScene.empty() && SceneSerializer::NormalizeSceneName(info->sceneName) != targetScene) return;

    for (int i = 0; i < indent; ++i) f << "  ";
    f << name << ":\n";
    int ci = indent + 1, ti = ci + 1;

    if (info && !info->tag.empty() && info->tag != "default") WriteKV(f, ci, "Tag", info->tag);
    if (info && !info->isActive) WriteKV(f, ci, "Active", "false");

    // Parent link if parent is in a different scene
    if (auto* h = reg.try_get<HierarchyComponent>(entity)) {
        if (h->parent != entt::null) {
            if (auto* pInfo = reg.try_get<InfoComponent>(h->parent)) {
                if (SceneSerializer::NormalizeSceneName(pInfo->sceneName) != targetScene) {
                    WriteKV(f, ci, "Parent", pInfo->name);
                }
            }
        }
    }

    // Transform
    auto* pos = reg.try_get<PositionComponent>(entity);
    auto* rot = reg.try_get<RotationComponent>(entity);
    auto* scale = reg.try_get<ScaleComponent>(entity);
    if (pos || rot || scale) {
        WriteComponentHeader(f, ci, "Transform");
        if (pos) WriteKV(f, ti, "Position", Vec3Str(pos->value));
        if (rot) WriteKV(f, ti, "Rotation", Vec3Str(glm::degrees(glm::eulerAngles(rot->value))));
        if (scale) WriteKV(f, ti, "Scale", Vec3Str(scale->value));
    }

    // MeshRenderer
    if (auto* mr = reg.try_get<MeshRendererComponent>(entity)) {
        WriteComponentHeader(f, ci, "Renderer");
        WriteKV(f, ti, "Model", mr->model ? mr->model->GetName() : "");
        if (!mr->shaderName.empty()) WriteKV(f, ti, "Shader", mr->shaderName);
        WriteKV(f, ti, "Order", std::to_string(mr->order));
        WriteKV(f, ti, "CastShadow", mr->castShadow ? "true" : "false");
        WriteKV(f, ti, "ReceiveShadow", mr->receiveShadow ? "true" : "false");
        WriteKV(f, ti, "Color", Vec4Str(mr->color));
        WriteKV(f, ti, "RenderMode", std::to_string((int)mr->renderMode));
    }

    // Material
    if (auto* mat = reg.try_get<AxisMaterialComponent>(entity)) {
        WriteComponentHeader(f, ci, "Material");
        WriteKV(f, ti, "Opacity", FloatStr(mat->desc.opacity));
        WriteKV(f, ti, "Roughness", FloatStr(mat->desc.pbr.roughness));
        WriteKV(f, ti, "Metallic", FloatStr(mat->desc.pbr.metallic));
        WriteKV(f, ti, "AO", FloatStr(mat->desc.pbr.ao));
        WriteKV(f, ti, "AlphaCutoff", FloatStr(mat->desc.alphaCutoff));
        WriteKV(f, ti, "Emission", Vec3Str(mat->desc.emission));
        WriteKV(f, ti, "UVScale", Vec2Str(mat->desc.uvScale));
        WriteKV(f, ti, "UVOffset", Vec2Str(mat->desc.uvOffset));

        if (!mat->desc.albedoPath.empty()) WriteKV(f, ti, "Albedo", SceneSerializer::NormalizePath(mat->desc.albedoPath));
        if (!mat->desc.normalPath.empty()) WriteKV(f, ti, "Normal", SceneSerializer::NormalizePath(mat->desc.normalPath));
        if (!mat->desc.metallicPath.empty()) WriteKV(f, ti, "MetallicMap", SceneSerializer::NormalizePath(mat->desc.metallicPath));
        if (!mat->desc.roughnessPath.empty()) WriteKV(f, ti, "RoughnessMap", SceneSerializer::NormalizePath(mat->desc.roughnessPath));
        if (!mat->desc.aoPath.empty()) WriteKV(f, ti, "AO_Map", SceneSerializer::NormalizePath(mat->desc.aoPath));
        if (!mat->desc.emissivePath.empty()) WriteKV(f, ti, "EmissiveMap", SceneSerializer::NormalizePath(mat->desc.emissivePath));
        if (!mat->desc.specularPath.empty()) WriteKV(f, ti, "SpecularMap", SceneSerializer::NormalizePath(mat->desc.specularPath));
    }

    // Animator
    if (auto* anim = reg.try_get<AnimationComponent>(entity)) {
        WriteComponentHeader(f, ci, "Animator");
        std::string anims;
        for (auto& a : anim->animations) anims += a + " ";
        if (!anims.empty()) WriteKV(f, ti, "Animation", anims);
        WriteKV(f, ti, "Speed", FloatStr(anim->speed));
        WriteKV(f, ti, "StartTime", FloatStr(anim->startTime));
        WriteKV(f, ti, "Rate", FloatStr(anim->rate));
        WriteKV(f, ti, "BlendFactor", FloatStr(anim->blendFactor));
    }

    // Lights
    if (auto* l = reg.try_get<DirectionalLightComponent>(entity)) {
        WriteComponentHeader(f, ci, "DirectionalLight");
        WriteKV(f, ti, "Active", l->active ? "true" : "false");
        WriteKV(f, ti, "Color", Vec3Str(l->color));
        WriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        WriteKV(f, ti, "Ambient", FloatStr(l->ambient));
        WriteKV(f, ti, "Diffuse", FloatStr(l->diffuse));
        WriteKV(f, ti, "Specular", FloatStr(l->specular));
    }
    if (auto* l = reg.try_get<PointLightComponent>(entity)) {
        WriteComponentHeader(f, ci, "PointLight");
        WriteKV(f, ti, "Active", l->active ? "true" : "false");
        WriteKV(f, ti, "Color", Vec3Str(l->color));
        WriteKV(f, ti, "Intensity", FloatStr(l->intensity));
        WriteKV(f, ti, "Radius", FloatStr(l->radius));
    }

    // Physics
    if (auto* rs = reg.try_get<RigidShapeComponent>(entity)) {
        WriteComponentHeader(f, ci, "RigidShape");
        WriteKV(f, ti, "Type", ShapeTypeToString(rs->type));
        WriteKV(f, ti, "Size", Vec3Str(rs->size));
        WriteKV(f, ti, "Radius", FloatStr(rs->radius));
        WriteKV(f, ti, "Height", FloatStr(rs->height));
        WriteKV(f, ti, "Friction", FloatStr(rs->friction));
        WriteKV(f, ti, "Restitution", FloatStr(rs->restitution));
        if (glm::length(rs->offset) > 0.0001f) WriteKV(f, ti, "Offset", Vec3Str(rs->offset));
        glm::vec3 euler = glm::degrees(glm::eulerAngles(rs->rotation));
        if (glm::length(euler) > 0.0001f) WriteKV(f, ti, "Rotation", Vec3Str(euler));
    }
    if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
        WriteComponentHeader(f, ci, "RigidBody");
        WriteKV(f, ti, "Mass", FloatStr(rb->mass));
        WriteKV(f, ti, "BodyType", rb->isStatic ? "STATIC" : (rb->isKinematic ? "KINEMATIC" : "DYNAMIC"));
        WriteKV(f, ti, "LinearDamping", FloatStr(rb->linearDamping));
        WriteKV(f, ti, "AngularDamping", FloatStr(rb->angularDamping));
        if (rb->isTrigger) WriteKV(f, ti, "IsTrigger", "true");
    }

    if (auto* frag = reg.try_get<FragmentComponent>(entity)) {
        WriteComponentHeader(f, ci, "Fragment");
        WriteKV(f, ti, "Path", SceneSerializer::NormalizePath(frag->path));
        if (!frag->overrides.empty()) {
            f << std::string((ti) * 2, ' ') << "Overrides:\n";
            std::stringstream ss(frag->overrides);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.empty()) continue;
                f << std::string((ti + 1) * 2, ' ') << line << "\n";
            }
        }
    }

    // Audio
    if (auto* audio = reg.try_get<AudioSourceComponent>(entity)) {
        WriteComponentHeader(f, ci, "AudioSource");
        if (!audio->resourceName.empty()) WriteKV(f, ti, "Audio", audio->resourceName);
        WriteKV(f, ti, "PlayOnAwake", audio->playOnAwake ? "true" : "false");
        WriteKV(f, ti, "Loop", audio->loop ? "true" : "false");
        WriteKV(f, ti, "Volume", FloatStr(audio->volume));
        WriteKV(f, ti, "Pitch", FloatStr(audio->pitch));
        WriteKV(f, ti, "Speed", FloatStr(audio->speed));
        WriteKV(f, ti, "Is3d", audio->is3D ? "true" : "false");
    }

    // Particle Emitter
    if (auto* pe = reg.try_get<ParticleEmitterComponent>(entity)) {
        WriteComponentHeader(f, ci, "ParticleEmitter");
        WriteKV(f, ti, "Active", pe->isActive ? "true" : "false");
        if (!pe->textureName.empty()) WriteKV(f, ti, "Texture", pe->textureName);
        if (!pe->customShader.empty()) WriteKV(f, ti, "Shader", pe->customShader);
        WriteKV(f, ti, "SpawnRate", FloatStr(pe->emitter.SpawnRate));
        WriteKV(f, ti, "Lifetime", FloatStr(pe->emitter.LifeTime));
        WriteKV(f, ti, "StartSize", FloatStr(pe->emitter.StartSize));
        WriteKV(f, ti, "EndSize", FloatStr(pe->emitter.EndSize));
        WriteKV(f, ti, "StartColor", Vec4Str(pe->emitter.StartColor));
        WriteKV(f, ti, "EndColor", Vec4Str(pe->emitter.EndColor));
        WriteKV(f, ti, "MinVelocity", Vec3Str(pe->emitter.MinVelocity));
        WriteKV(f, ti, "MaxVelocity", Vec3Str(pe->emitter.MaxVelocity));
    }

    // PostProcess
    if (auto* pp = reg.try_get<PostProcessComponent>(entity)) {
        WriteComponentHeader(f, ci, "PostProcess");
        WriteKV(f, ti, "Active", pp->enabled ? "true" : "false");
        std::string effects;
        for (auto& eff : pp->effects) {
            effects += eff.shaderName + ":" + std::to_string(eff.priority) + ":" + 
                       FloatStr(eff.x) + ":" + FloatStr(eff.y) + ":" + 
                       FloatStr(eff.w) + ":" + FloatStr(eff.h) + ":" +
                       (eff.affectUI ? "1" : "0") + " ";
        }
        if (!effects.empty()) WriteKV(f, ti, "Effects", effects);
    }

    // UI
    if (auto* uit = reg.try_get<UITransformComponent>(entity)) {
        WriteComponentHeader(f, ci, "UITransform");
        WriteKV(f, ti, "position", Vec2PercentStr(uit->position, uit->positionIsPercent));
        WriteKV(f, ti, "size", Vec2PercentStr(uit->size, uit->sizeIsPercent));
        WriteKV(f, ti, "zIndex", std::to_string(uit->zIndex));
        WriteKV(f, ti, "pivot", Vec2Str(uit->pivot));
        WriteKV(f, ti, "anchorMin", Vec2PercentStr(uit->anchorMin, uit->anchorMinIsPercent));
        WriteKV(f, ti, "anchorMax", Vec2PercentStr(uit->anchorMax, uit->anchorMaxIsPercent));
        WriteKV(f, ti, "offsetMin", Vec2PercentStr(uit->offsetMin, uit->offsetMinIsPercent));
        WriteKV(f, ti, "offsetMax", Vec2PercentStr(uit->offsetMax, uit->offsetMaxIsPercent));
    }

    if (auto* uir = reg.try_get<UIRendererComponent>(entity)) {
        WriteComponentHeader(f, ci, "UIRenderer");
        WriteKV(f, ti, "color", Vec4Str(uir->color));
        if (uir->texture) WriteKV(f, ti, "texture", SceneSerializer::NormalizePath(uir->texture->path));
        if (!uir->shaderName.empty()) WriteKV(f, ti, "shader", uir->shaderName);
    }

    if (auto* uitext = reg.try_get<UITextComponent>(entity)) {
        WriteComponentHeader(f, ci, "UIText");
        WriteKV(f, ti, "text", "\"" + uitext->text + "\"");
        WriteKV(f, ti, "color", Vec4Str(uitext->color));
        WriteKV(f, ti, "scale", FloatStr(uitext->scale));
        if (uitext->font) WriteKV(f, ti, "fontSize", std::to_string(uitext->font->GetFontSize()));
        WriteKV(f, ti, "alignment", (uitext->alignment == TextAlignment::Center ? "Center" : (uitext->alignment == TextAlignment::Right ? "Right" : "Left")));
        WriteKV(f, ti, "wordWrap", uitext->wordWrap ? "true" : "false");
        WriteKV(f, ti, "maxWidth", FloatStr(uitext->maxWidth));
        if (!uitext->fontName.empty()) WriteKV(f, ti, "font", uitext->fontName);
    }

    if (auto* uif = reg.try_get<UIFlexLayoutComponent>(entity)) {
        WriteComponentHeader(f, ci, "UIFlex");
        WriteKV(f, ti, "direction", uif->direction == FlexDirection::Row ? "Row" : "Column");
        WriteKV(f, ti, "spacing", FloatStr(uif->spacing));
        WriteKV(f, ti, "autoSize", uif->autoSize ? "true" : "false");
        WriteKV(f, ti, "padding", Vec4Str(uif->padding));
    }

    if (auto* sky = reg.try_get<SkyboxRenderComponent>(entity)) {
        WriteComponentHeader(f, ci, "SkyboxRenderer");
        if (sky->skybox) WriteKV(f, ti, "Skybox", sky->skybox->GetName());
        if (!sky->shaderName.empty()) WriteKV(f, ti, "Shader", sky->shaderName);
    }

    // Parent
    if (auto* h = reg.try_get<HierarchyComponent>(entity)) {
        if (h->parent != entt::null) {
            if (auto* pInfo = reg.try_get<InfoComponent>(h->parent)) {
                // If parent belongs to a DIFFERENT scene, we MUST write the Parent name
                // so the deserializer can link them back.
                if (SceneSerializer::NormalizeSceneName(pInfo->sceneName) != targetScene) {
                    WriteKV(f, indent + 1, "Parent", pInfo->name);
                }
            }
        }
        for (auto child : h->children) {
            if (auto* cInfo = reg.try_get<InfoComponent>(child)) {
                if (!cInfo->sceneName.empty() && !targetScene.empty() && SceneSerializer::NormalizeSceneName(cInfo->sceneName) != targetScene) continue;
            }
            
            // CRITICAL: If the current entity is a Fragment, DO NOT serialize its children!
            // They will be re-instantiated by the FragmentSystem on load.
            if (reg.all_of<FragmentComponent>(entity)) continue;

            SerializeEntity(f, reg, child, indent + 1, sceneName);
        }
    }
}

bool SceneSerializer::Serialize(const std::string& filepath, Scene& scene, ResourceManager& res, const std::string& sceneName) {
    std::ofstream f(filepath);
    if (!f.is_open()) return false;
    f << "axis_scene:\n";

    std::string normName = SceneSerializer::NormalizeSceneName(sceneName);
    auto& sl = ServiceLocator::Instance();
    auto& configMgr = sl.Require<ConfigManager>();
    auto* sceneMgr = sl.Resolve<SceneManager>();
    const SceneRecord* rec = sceneMgr ? sceneMgr->GetSceneByName(normName) : nullptr;

    if (rec && rec->hasConfig) {
        AppConfig cfg = configMgr.GetConfig();
        f << "  Config:\n";
        WriteKV(f, 2, "WINDOW_WIDTH", std::to_string(cfg.window.width));
        WriteKV(f, 2, "WINDOW_HEIGHT", std::to_string(cfg.window.height));
    }

    UsedResources ur;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName) continue;
        
        bool isRootInScene = true;
        if (auto* h = scene.registry.try_get<HierarchyComponent>(entity)) {
            if (h->parent != entt::null) {
                if (auto* pInfo = scene.registry.try_get<InfoComponent>(h->parent)) {
                    if (SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName) isRootInScene = false;
                }
            }
        }
        if (isRootInScene) CollectResources(scene.registry, entity, ur, normName);
    }

    f << "  Resources:\n";
    auto IsOwnedByOther = [&](const std::string& name, const std::string& type) {
        if (!sceneMgr) return false;
        for (const auto& otherRec : sceneMgr->GetAllScenes()) {
            if (SceneSerializer::NormalizeSceneName(otherRec.name) == normName) continue;
            
            const std::vector<std::string>* list = nullptr;
            if (type == "Shader") list = &otherRec.ownedShaders;
            else if (type == "Model") list = &otherRec.ownedModels;
            else if (type == "Texture") list = &otherRec.ownedTextures;
            else if (type == "Font") list = &otherRec.ownedFonts;
            else if (type == "Skybox") list = &otherRec.ownedSkyboxes;
            else if (type == "Animation") list = &otherRec.ownedAnimations;
            else if (type == "Audio" || type == "Sound") list = &otherRec.ownedSounds;
            
            if (list && std::find(list->begin(), list->end(), name) != list->end()) return true;
        }
        return false;
    };

    for (const auto& def : res.GetResourceDefinitions()) {
        bool used = false;
        if (def.type == "Shader") used = ur.shaders.count(def.name) || (rec && std::find(rec->ownedShaders.begin(), rec->ownedShaders.end(), def.name) != rec->ownedShaders.end());
        else if (def.type == "Model") used = ur.models.count(def.name) || (rec && std::find(rec->ownedModels.begin(), rec->ownedModels.end(), def.name) != rec->ownedModels.end());
        else if (def.type == "Texture") {
            std::string relPath = SceneSerializer::NormalizePath(def.properties.count("Path") ? def.properties.at("Path") : "");
            used = ur.textures.count(def.name) || ur.textures.count(relPath) || (rec && std::find(rec->ownedTextures.begin(), rec->ownedTextures.end(), def.name) != rec->ownedTextures.end());
        }
        else if (def.type == "Font") used = ur.fonts.count(def.name) || (rec && std::find(rec->ownedFonts.begin(), rec->ownedFonts.end(), def.name) != rec->ownedFonts.end());
        else if (def.type == "Skybox") used = ur.skyboxes.count(def.name) || (rec && std::find(rec->ownedSkyboxes.begin(), rec->ownedSkyboxes.end(), def.name) != rec->ownedSkyboxes.end());
        else if (def.type == "Animation") used = ur.animations.count(def.name) || (rec && std::find(rec->ownedAnimations.begin(), rec->ownedAnimations.end(), def.name) != rec->ownedAnimations.end());
        else if (def.type == "Audio" || def.type == "Sound") used = ur.sounds.count(def.name) || (rec && std::find(rec->ownedSounds.begin(), rec->ownedSounds.end(), def.name) != rec->ownedSounds.end());
        
        if (!used) continue;
        if (IsOwnedByOther(def.name, def.type)) continue;

        f << "    " << def.type << ":\n";
        f << "      Name: " << def.name << "\n";
        for (const auto& prop : def.properties) {
            std::string val = prop.second;
            if (prop.first == "Path" || prop.first == "Vertex" || prop.first == "Fragment" || prop.first == "Geometry" || 
                prop.first == "Albedo" || prop.first == "Normal" || prop.first == "MetallicMap" || prop.first == "RoughnessMap" ||
                prop.first == "Right" || prop.first == "Left" || prop.first == "Top" || prop.first == "Bottom" || prop.first == "Front" || prop.first == "Back") {
                val = SceneSerializer::NormalizePath(val);
            }
            WriteKV(f, 3, prop.first, val);
        }
    }

    f << "  Entities:\n";
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (!normName.empty() && SceneSerializer::NormalizeSceneName(info.sceneName) != normName) continue;
        
        bool isRootInScene = true;
        if (auto* h = scene.registry.try_get<HierarchyComponent>(entity)) {
            if (h->parent != entt::null) {
                if (auto* pInfo = scene.registry.try_get<InfoComponent>(h->parent)) {
                    if (SceneSerializer::NormalizeSceneName(pInfo->sceneName) == normName) isRootInScene = false;
                }
            }
        }
        if (isRootInScene) SerializeEntity(f, scene.registry, entity, 2, normName);
    }
    return true;
}



