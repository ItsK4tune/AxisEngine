#include <core/app/application.h>
#include <core/app/config_loader.h>
#include <systems/window/io_handler.h>
#include <systems/window/monitor_manager.h>
#include <ecs/entity_manager.h>
#include <systems/physics/physics_loader.h>
#include <scene/component_loader.h>
#include <scene/handlers/scene_validator.h>
#include <scene/scene_serializer.h>
#include <core/utils/filesystem.h>
#include <core/utils/logger.h>
#include <core/utils/yaml_parser.h>

SceneLoadResult SceneSerializer::Deserialize(const std::string &filepath, Scene &scene, ResourceManager &res, IPhysicsWorld &phys, SoundPlayer &sound, EngineContext ctx)
{
    SceneLoadResult result;
    std::string fullPath = FileSystem::getPath(filepath);
    auto roots = YAMLParser::Parse(fullPath);
    if (roots.empty())
    {
        LOGGER_ERROR("SceneSerializer") << "Failed to parse AXS file or file is empty: " << fullPath;
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

    for (auto &root : activeRoots)
    {
        if (root.key == "Config")
        {
            AppConfig tempConfig = ctx.IsValid() ? ctx.runtime->GetConfig() : AppConfig{};
            bool applyWindow = false;

            for (auto &cfgNode : root.children)
            {
                if (cfgNode.key.find("WINDOW") != std::string::npos)
                {
                    applyWindow = true;
                }

                std::stringstream ss1;
                ss1 << cfgNode.key << " " << cfgNode.value;
                ConfigLoader::LoadConfig(ss1, tempConfig);

                std::stringstream ss2;
                ss2 << cfgNode.key << " " << cfgNode.value;
                ConfigLoader::LoadConfig(ss2, ctx);
            }

            if (applyWindow && ctx.IsValid())
            {
                ctx.io->GetMonitorManager().SetWindowConfiguration(
                    tempConfig.width, tempConfig.height,
                    (WindowMode)tempConfig.windowMode,
                    tempConfig.monitorIndex, tempConfig.refreshRate);
            }

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
                    std::string vs = resNode.GetChildValue("VS");
                    std::string fs = resNode.GetChildValue("FS");
                    std::string gs = resNode.GetChildValue("GS");
                    res.LoadShader(name, vs, fs, gs);
                    result.loadedShaders.push_back(name);
                }
                else if (resNode.key == "Model")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    bool isStatic = resNode.GetChildValue("Static") == "1" || resNode.GetChildValue("Static") == "true";
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
                        resNode.GetChildValue("Right"),
                        resNode.GetChildValue("Left"),
                        resNode.GetChildValue("Top"),
                        resNode.GetChildValue("Bottom"),
                        resNode.GetChildValue("Front"),
                        resNode.GetChildValue("Back")};
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
                else if (resNode.key == "Sound")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    if (ctx.IsValid())
                        res.LoadSound(name, path, nullptr);
                    result.loadedSounds.push_back(name);
                }
            }
        }
        else if (root.key == "Entities")
        {
            for (auto &entNode : root.children)
            {
                std::string entityName = entNode.key;
                std::string entityTag = entNode.GetChildValue("Tag", "default");

                bool duplicate = false;
                auto view = scene.registry.view<InfoComponent>();
                for (auto e : view)
                {
                    auto &info = view.get<InfoComponent>(e);
                    if (info.name == entityName && info.tag == entityTag && info.sceneName == sceneName)
                    {
                        LOGGER_WARN("SceneSerializer") << "Duplicate entity skipped: name='" << entityName
                                                       << "' tag='" << entityTag << "' scene='" << sceneName << "'";
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate)
                    continue;

                entt::entity currentEntity = EntityManager::CreateEntity(scene, entityName, entityTag);
                uint32_t layer = std::stoul(entNode.GetChildValue("Layer", "1"));

                auto &info = scene.registry.get<InfoComponent>(currentEntity);
                info.sceneName = sceneName;
                info.layer = layer;
                result.entities.push_back(currentEntity);

                YAMLNode *tNode = nullptr;
                for (auto &child : entNode.children)
                {
                    if (child.key == "Component" && child.value == "Transform")
                    {
                        tNode = &child;
                        break;
                    }
                }

                if (tNode)
                {
                    std::stringstream ss;
                    ss << tNode->GetChildValue("Position", "0 0 0") << " "
                       << tNode->GetChildValue("Rotation", "0 0 0") << " "
                       << tNode->GetChildValue("Scale", "1 1 1");

                    float x, y, z, rx, ry, rz, sx, sy, sz;
                    ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
                    
                    glm::vec3 pos(x, y, z);
                    glm::quat rot = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
                    glm::vec3 scl(sx, sy, sz);

                    if (auto* p = scene.registry.try_get<PositionComponent>(currentEntity)) p->value = p->prev = pos;
                    if (auto* r = scene.registry.try_get<RotationComponent>(currentEntity)) r->value = r->prev = rot;
                    if (auto* s = scene.registry.try_get<ScaleComponent>(currentEntity)) s->value = s->prev = scl;
                    
                    if (auto* w = scene.registry.try_get<WorldTransformComponent>(currentEntity))
                    {
                        w->isDirty = true;
                        w->worldMatrix = glm::mat4(1.0f);
                        w->prevWorldMatrix = glm::mat4(1.0f);
                    }
                }

                if (auto *pNode = entNode.GetChild("Parent"))
                {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                ComponentLoader::InitializeDefaultLoaders();

                for (auto &compNode : entNode.children)
                {
                    if (compNode.key != "Component")
                        continue;
                    std::string compType = compNode.value;

                    if (!ComponentLoader::Load(compType, scene, currentEntity, compNode, res, phys, ctx))
                    {
                        LOGGER_WARN("SceneSerializer") << "No ComponentLoader registered for component type: " << compType;
                    }
                }
            }
        }
    }

    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);
    SceneHandlers::SceneValidator::ValidateCamera(scene, ctx);

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS file: " << fullPath;
    return result;
}
