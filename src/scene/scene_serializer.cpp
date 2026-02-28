#include <scene/scene_serializer.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <app/config_loader.h>
#include <app/monitor_manager.h>
#include <scene/component_loader.h>
#include <physic/physics_loader.h>
#include <app/application.h>
#include <scene/handlers/scene_validator.h>
#include <utils/yaml_parser.h>

SceneLoadResult SceneSerializer::Deserialize(const std::string &filepath, Scene &scene, ResourceManager &res, IPhysicsWorld &phys, SoundPlayer &sound, Application *app)
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
            AppConfig tempConfig = app ? app->GetConfig() : AppConfig{};
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
                ConfigLoader::LoadConfig(ss2, app);
            }

            if (applyWindow && app)
            {
                app->GetMonitorManager().SetWindowConfiguration(
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
                    if (app)
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

                entt::entity currentEntity = scene.createEntity();
                uint32_t layer = std::stoul(entNode.GetChildValue("Layer", "1"));

                auto &info = scene.registry.emplace<InfoComponent>(currentEntity, entityName, entityTag);
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
                    auto &t = scene.registry.get_or_emplace<TransformComponent>(currentEntity);
                    std::stringstream ss;
                    ss << tNode->GetChildValue("Position", "0 0 0") << " "
                       << tNode->GetChildValue("Rotation", "0 0 0") << " "
                       << tNode->GetChildValue("Scale", "1 1 1");

                    float x, y, z, rx, ry, rz, sx, sy, sz;
                    ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
                    t.position = glm::vec3(x, y, z);
                    t.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
                    t.scale = glm::vec3(sx, sy, sz);

                    t.prevPosition = t.position;
                    t.prevRotation = t.rotation;
                    t.prevScale = t.scale;
                }

                if (auto *pNode = entNode.GetChild("Parent"))
                {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto &compNode : entNode.children)
                {
                    if (compNode.key != "Component")
                        continue;
                    std::string compType = compNode.value;

                    if (compType == "Renderer")
                        ComponentLoader::LoadRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "Animator")
                        ComponentLoader::LoadAnimator(scene, currentEntity, compNode, res);
                    else if (compType == "Camera")
                        ComponentLoader::LoadCamera(scene, currentEntity, compNode);
                    else if (compType == "LightDir")
                        ComponentLoader::LoadLightDir(scene, currentEntity, compNode);
                    else if (compType == "LightPoint")
                        ComponentLoader::LoadLightPoint(scene, currentEntity, compNode);
                    else if (compType == "LightSpot")
                        ComponentLoader::LoadLightSpot(scene, currentEntity, compNode);
                    else if (compType == "UITransform")
                        ComponentLoader::LoadUITransform(scene, currentEntity, compNode);
                    else if (compType == "UIRenderer")
                        ComponentLoader::LoadUIRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "UIText")
                        ComponentLoader::LoadUIText(scene, currentEntity, compNode, res);
                    else if (compType == "SkyboxRenderer")
                        ComponentLoader::LoadSkyboxRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "Script")
                        ComponentLoader::LoadScript(scene, currentEntity, compNode, app);
                    else if (compType == "AudioSource")
                        ComponentLoader::LoadAudioSource(scene, currentEntity, compNode);
                    else if (compType == "VideoPlayer")
                        ComponentLoader::LoadVideoPlayer(scene, currentEntity, compNode);
                    else if (compType == "ParticleEmitter")
                        ComponentLoader::LoadParticleEmitter(scene, currentEntity, compNode, res);
                    else if (compType == "Material")
                        ComponentLoader::LoadMaterial(scene, currentEntity, compNode, res);
                    else if (compType == "LOD")
                        ComponentLoader::LoadLOD(scene, currentEntity, compNode, res);
                    else if (compType == "RigidBody")
                        PhysicsLoader::LoadRigidBody(scene, currentEntity, compNode, phys);
                }
            }
        }
    }

    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);
    SceneHandlers::SceneValidator::ValidateCamera(scene, app);

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS file: " << fullPath;
    return result;
}
