#include <scene/scene_serializer.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <app/config_loader.h>
#include <app/monitor_manager.h>
#include <interface/window/i_window.h>
#include <scene/component_loader.h>
#include <physic/physics_loader.h>
#include <app/application.h>
#include <scene/handlers/scene_validator.h>
#include <fstream>
#include <sstream>

YAMLNode* GetNodeAtPath(std::vector<YAMLNode>& roots, const std::vector<size_t>& path) {
    if (path.empty()) return nullptr;
    YAMLNode* current = &roots[path[0]];
    for (size_t i = 1; i < path.size(); ++i) {
        current = &current->children[path[i]];
    }
    return current;
}

std::vector<YAMLNode> SceneSerializer::ParseAXS(const std::string& filepath)
{
    std::vector<YAMLNode> roots;
    std::ifstream file(filepath);
    if (!file.is_open()) return roots;

    struct IndentLevel {
        int indent;
        std::vector<size_t> path;
    };
    std::vector<IndentLevel> stack;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        int indent = 0;
        while (indent < line.length() && (line[indent] == ' ' || line[indent] == '\t')) {
            indent++;
        }
        
        std::string content = line.substr(indent);
        if (content.empty() || content[0] == '#') continue;

        auto colonPos = content.find(':');
        if (colonPos == std::string::npos) continue; 

        std::string key = content.substr(0, colonPos);
        std::string value = "";
        if (colonPos + 1 < content.length()) {
            value = content.substr(colonPos + 1);
            size_t start = value.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) {
                value = value.substr(start);
                size_t end = value.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) value = value.substr(0, end + 1);
            } else {
                value = "";
            }
        }

        YAMLNode newNode{key, value, {}};

        while (!stack.empty() && stack.back().indent >= indent) {
            stack.pop_back();
        }

        if (stack.empty()) {
            roots.push_back(newNode);
            stack.push_back({indent, { roots.size() - 1 }});
        } else {
            YAMLNode* parent = GetNodeAtPath(roots, stack.back().path);
            parent->children.push_back(newNode);
            
            std::vector<size_t> currentPath = stack.back().path;
            currentPath.push_back(parent->children.size() - 1);
            stack.push_back({indent, currentPath});
        }
    }
    return roots;
}

void SceneSerializer::WriteAXS(std::ofstream& out, const YAMLNode& node, int indent)
{
    for (int i = 0; i < indent; ++i) out << "  ";
    out << node.key << ":";
    if (!node.value.empty()) out << " " << node.value;
    out << "\n";
    
    for (const auto& child : node.children) {
        WriteAXS(out, child, indent + 1);
    }
}

void SceneSerializer::Serialize(Scene& scene, const std::string& filepath)
{
    std::ofstream out(filepath);
    out << "axis_scene:\n";
}

std::vector<entt::entity> SceneSerializer::Deserialize(const std::string& filepath, Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app)
{
    std::string fullPath = FileSystem::getPath(filepath);
    auto roots = ParseAXS(fullPath);
    if (roots.empty()) {
        LOGGER_ERROR("SceneSerializer") << "Failed to parse AXS file or file is empty: " << fullPath;
        return {};
    }

    std::vector<entt::entity> loadedEntities;
    std::map<entt::entity, std::vector<std::string>> deferredChildren;

    std::vector<YAMLNode> activeRoots = roots;
    if (roots.size() == 1 && roots[0].key == "axis_scene") {
        activeRoots = roots[0].children;
    }

    for (auto& root : activeRoots) {
        if (root.key == "Config") {
            AppConfig tempConfig = app ? app->GetConfig() : AppConfig{};
            bool applyWindow = false;

            for (auto& cfgNode : root.children) {
                if (cfgNode.key.find("WINDOW") != std::string::npos) {
                    applyWindow = true;
                }

                std::stringstream ss1;
                ss1 << cfgNode.key << " " << cfgNode.value;
                ConfigLoader::LoadConfig(ss1, tempConfig);

                std::stringstream ss2;
                ss2 << cfgNode.key << " " << cfgNode.value;
                ConfigLoader::LoadConfig(ss2, app);
            }

            if (applyWindow && app) {
                app->GetMonitorManager().SetWindowConfiguration(
                    tempConfig.width, tempConfig.height, 
                    (WindowMode)tempConfig.windowMode, 
                    tempConfig.monitorIndex, tempConfig.refreshRate);
            }
        }
        else if (root.key == "Resources") {
            for (auto& resNode : root.children) {
                if (resNode.key == "Shader") {
                    std::string name = resNode.GetChildValue("Name");
                    std::string vs = resNode.GetChildValue("VS");
                    std::string fs = resNode.GetChildValue("FS");
                    std::string gs = resNode.GetChildValue("GS");
                    res.LoadShader(name, vs, fs, gs);
                }
                else if (resNode.key == "Model") {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    bool isStatic = resNode.GetChildValue("Static") == "1" || resNode.GetChildValue("Static") == "true";
                    res.LoadModel(name, path, isStatic);
                }
                else if (resNode.key == "Texture") {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    res.LoadTexture(name, path);
                }
                else if (resNode.key == "Font") {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    unsigned int size = std::stoul(resNode.GetChildValue("Size", "16"));
                    res.LoadFont(name, path, size);
                }
                else if (resNode.key == "Skybox") {
                    std::string name = resNode.GetChildValue("Name");
                    std::vector<std::string> faces = {
                        resNode.GetChildValue("Right"),
                        resNode.GetChildValue("Left"),
                        resNode.GetChildValue("Top"),
                        resNode.GetChildValue("Bottom"),
                        resNode.GetChildValue("Front"),
                        resNode.GetChildValue("Back")
                    };
                    res.LoadSkybox(name, faces);
                }
            }
        }
        else if (root.key == "Entities") {
            for (auto& entNode : root.children) {
                entt::entity currentEntity = scene.createEntity();
                std::string entityName = entNode.key;
                std::string entityTag = entNode.GetChildValue("Tag", "default");
                uint32_t layer = std::stoul(entNode.GetChildValue("Layer", "1"));
                
                auto& info = scene.registry.emplace<InfoComponent>(currentEntity, entityName, entityTag);
                info.layer = layer;
                loadedEntities.push_back(currentEntity);

                YAMLNode* tNode = nullptr;
                for (auto& child : entNode.children) {
                    if (child.key == "Component" && child.value == "Transform") {
                        tNode = &child;
                        break;
                    }
                }

                if (tNode) {
                    auto& t = scene.registry.get_or_emplace<TransformComponent>(currentEntity);
                    std::stringstream ss;
                    ss << tNode->GetChildValue("Position", "0 0 0") << " "
                       << tNode->GetChildValue("Rotation", "0 0 0") << " "
                       << tNode->GetChildValue("Scale", "1 1 1");
                    
                    float x,y,z, rx,ry,rz, sx,sy,sz;
                    ss >> x >> y >> z >> rx >> ry >> rz >> sx >> sy >> sz;
                    t.position = glm::vec3(x, y, z);
                    t.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
                    t.scale = glm::vec3(sx, sy, sz);
                }
                
                if (auto* pNode = entNode.GetChild("Parent")) {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto& compNode : entNode.children) {
                    if (compNode.key != "Component") continue;
                    std::string compType = compNode.value;

                    if (compType == "Transform") { // Handled above, but if we strictly use Component: Transform
                        // Can skip or re-implement if user writes Component: Transform
                    }
                    else if (compType == "Renderer") ComponentLoader::LoadRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "Animator") ComponentLoader::LoadAnimator(scene, currentEntity, compNode, res);
                    else if (compType == "Camera") ComponentLoader::LoadCamera(scene, currentEntity, compNode);
                    else if (compType == "LightDir") ComponentLoader::LoadLightDir(scene, currentEntity, compNode);
                    else if (compType == "LightPoint") ComponentLoader::LoadLightPoint(scene, currentEntity, compNode);
                    else if (compType == "LightSpot") ComponentLoader::LoadLightSpot(scene, currentEntity, compNode);
                    else if (compType == "UITransform") ComponentLoader::LoadUITransform(scene, currentEntity, compNode);
                    else if (compType == "UIRenderer") ComponentLoader::LoadUIRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "UIText") ComponentLoader::LoadUIText(scene, currentEntity, compNode, res);
                    else if (compType == "SkyboxRenderer") ComponentLoader::LoadSkyboxRenderer(scene, currentEntity, compNode, res);
                    else if (compType == "Script") ComponentLoader::LoadScript(scene, currentEntity, compNode, app);
                    else if (compType == "AudioSource") ComponentLoader::LoadAudioSource(scene, currentEntity, compNode);
                    else if (compType == "VideoPlayer") ComponentLoader::LoadVideoPlayer(scene, currentEntity, compNode);
                    else if (compType == "ParticleEmitter") ComponentLoader::LoadParticleEmitter(scene, currentEntity, compNode, res);
                    else if (compType == "Material") ComponentLoader::LoadMaterial(scene, currentEntity, compNode);
                    else if (compType == "LOD") ComponentLoader::LoadLOD(scene, currentEntity, compNode, res);
                    else if (compType == "Camera") ComponentLoader::LoadCamera(scene, currentEntity, compNode);
                    else if (compType == "RigidBody") PhysicsLoader::LoadRigidBody(scene, currentEntity, compNode, phys);
                }
            }
        }
    }

    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);
    SceneHandlers::SceneValidator::ValidateCamera(scene, app);

    LOGGER_INFO("SceneSerializer") << "Finished parsing AXS file: " << fullPath;
    return loadedEntities;
}
