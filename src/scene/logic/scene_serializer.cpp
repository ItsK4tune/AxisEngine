#include <ecs/unit/core_components.h>
#include <core/app/application.h>
#include <core/type/app_config.h>
#include <core/logic/config_loader.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/logic/entity_manager.h>
#include <physics/logic/physics_loader.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_validator.h>
#include <scene/logic/scene_serializer.h>
#include <core/app/runtime_core.h>
#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/loader_utils.h>
#include <audio/logic/audio_service.h>
#include <audio/interface/i_audio_engine.h>

SceneLoadResult SceneSerializer::Deserialize(const std::string &filepath, Scene &scene, ResourceManager &res, IPhysicsWorld* phys, AudioService &sound)
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
            auto& sl = ServiceLocator::Instance();
            auto& configMgr = sl.Require<ConfigManager>();
            AppConfig tempConfig = configMgr.GetConfig();
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
            }
            
            configMgr.UpdateConfig(tempConfig);

            if (applyWindow)
            {
                auto io = ServiceLocator::Instance().Resolve<IOHandler>();
                if (io) {
                    io->GetMonitorManager().SetWindowConfiguration(
                        tempConfig.width, tempConfig.height,
                        (WindowMode)tempConfig.windowMode,
                        tempConfig.monitorIndex, tempConfig.refreshRate);
                }
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
                    LoaderUtils::ValidateKeys(resNode, {"Name", "vertex", "geometry", "fragment", "VS", "FS", "GS"}, "Resource:Shader");
                    std::string name = resNode.GetChildValue("Name");
                    
                    // Support both new names and legacy names
                    std::string vs = resNode.GetChildValue("vertex");
                    if (vs.empty()) vs = resNode.GetChildValue("VS");
                    
                    std::string fs = resNode.GetChildValue("fragment");
                    if (fs.empty()) fs = resNode.GetChildValue("FS");
                    
                    std::string gs = resNode.GetChildValue("geometry");
                    if (gs.empty()) gs = resNode.GetChildValue("GS");
                    
                    res.LoadShader(name, vs, fs, gs);
                    result.loadedShaders.push_back(name);
                }
                else if (resNode.key == "Model")
                {
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Path", "Static"}, "Resource:Model");
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    bool isStatic = resNode.GetChildValue("Static") == "1" || resNode.GetChildValue("Static") == "true";
                    res.LoadModel(name, path, isStatic);
                    result.loadedModels.push_back(name);
                }
                else if (resNode.key == "Texture")
                {
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Path"}, "Resource:Texture");
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    res.LoadTexture(name, path);
                    result.loadedTextures.push_back(name);
                }
                else if (resNode.key == "Font")
                {
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Path", "Size"}, "Resource:Font");
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    unsigned int size = std::stoul(resNode.GetChildValue("Size", "16"));
                    res.LoadFont(name, path, size);
                    result.loadedFonts.push_back(name);
                }
                else if (resNode.key == "Skybox")
                {
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Right", "Left", "Top", "Bottom", "Front", "Back"}, "Resource:Skybox");
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
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Path", "Model"}, "Resource:Animation");
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    std::string model = resNode.GetChildValue("Model");
                    res.LoadAnimation(name, path, model);
                    result.loadedAnimations.push_back(name);
                }
                else if (resNode.key == "Audio")
                {
                    LoaderUtils::ValidateKeys(resNode, {"Name", "Path", "Volume", "Pitch", "Pan", "Speed"}, "Resource:Audio");
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    
                    res.LoadSound(name, path, sound.GetEngine());
                    auto source = res.GetSound(name);
                    if (source)
                    {
                        float vol = std::stof(resNode.GetChildValue("Volume", "1.0"));
                        float pitch = std::stof(resNode.GetChildValue("Pitch", "1.0"));
                        float pan = std::stof(resNode.GetChildValue("Pan", "0.0"));
                        float speed = std::stof(resNode.GetChildValue("Speed", "1.0"));

                        source->SetDefaultVolume(vol);
                        source->SetDefaultPitch(pitch);
                        source->SetDefaultPan(pan);
                        source->SetDefaultSpeed(speed);
                    }
                    result.loadedSounds.push_back(name);
                }
            }
        }
        else if (root.key == "Entities")
        {
            ComponentLoader::InitializeDefaultLoaders();
            for (auto &entNode : root.children)
            {
                LoaderUtils::ValidateKeys(entNode, {"Tag", "Layer", "Parent", "Component"}, "Entity:" + entNode.key);
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
                        LoaderUtils::ValidateKeys(child, {"Position", "Rotation", "Scale"}, "Component:Transform");
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

                    float x = 0, y = 0, z = 0, rx = 0, ry = 0, rz = 0, sx = 1, sy = 1, sz = 1;
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

 

                for (auto &compNode : entNode.children)
                {
                    if (compNode.key != "Component")
                        continue;
                    std::string compType = compNode.value;

                    if (!ComponentLoader::Load(compType, scene, currentEntity, compNode, res, phys))
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
    SceneHandlers::SceneValidator::ValidateCamera(scene);

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS file: " << fullPath;
    return result;
}






