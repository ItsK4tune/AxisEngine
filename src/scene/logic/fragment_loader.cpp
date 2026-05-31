#include <engine/scene/logic/fragment_loader.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <scene/logic/component_loader.h>
#include <scene/logic/scene_validator.h>

std::map<std::string, entt::entity> FragmentLoader::Instantiate(const FragmentAsset& asset, Scene& scene,
                                                                entt::entity parent, ResourceManager& res,
                                                                IPhysicsWorld* phys, AudioService* sound,
                                                                const YAMLNode* overrideNode)
{
    std::map<std::string, entt::entity> instantiatedEntities;
    std::map<entt::entity, std::vector<std::string>> deferredChildren;

    // We use a temporary scene name or the fragment path
    std::string fragmentName = asset.path;

    for (auto& root : asset.rootNodes)
    {
        if (root.key == "Resources")
        {
            for (auto& resNode : root.children)
            {
                if (resNode.key == "Shader")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string vs = resNode.GetChildValue("vertex", resNode.GetChildValue("VS"));
                    std::string fs = resNode.GetChildValue("fragment", resNode.GetChildValue("FS"));
                    std::string fs_override = resNode.GetChildValue("FS");
                    if (fs.empty())
                        fs = fs_override;
                    std::string gs = resNode.GetChildValue("geometry", resNode.GetChildValue("GS"));
                    res.LoadShader(name, vs, fs, gs);
                }
                else if (resNode.key == "Model")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    bool isStatic = resNode.GetChildValue("Static") == "1" || resNode.GetChildValue("Static") == "true";
                    res.LoadModel(name, path, isStatic);
                }
                else if (resNode.key == "Texture")
                {
                    std::string name = resNode.GetChildValue("Name");
                    std::string path = resNode.GetChildValue("Path");
                    res.LoadTexture(name, path);
                }
                else if (resNode.key == "Audio")
                {
                    if (sound)
                    {
                        std::string name = resNode.GetChildValue("Name");
                        std::string path = resNode.GetChildValue("Path");
                        res.LoadSound(name, path, sound->GetEngine());
                    }
                    else
                    {
                        LOGGER_WARN("FragmentLoader") << "Skipping audio resource load: No AudioService available";
                    }
                }
                // Add more resource types if needed
            }
        }
        else if (root.key == "Entities")
        {
            std::string namePrefix = "";
            std::string tagPrefix = "";
            if (parent != entt::null && scene.registry.all_of<InfoComponent>(parent))
            {
                auto& parentInfo = scene.registry.get<InfoComponent>(parent);
                namePrefix = parentInfo.name + ".";
                tagPrefix = parentInfo.tag + ".";
            }

            ComponentLoader::InitializeDefaultLoaders();
            for (auto& entNodeRaw : root.children)
            {
                YAMLNode entNode = entNodeRaw;  // Clone node to apply overrides
                std::string entityName = entNode.key;

                // Apply Overrides if any
                if (overrideNode)
                {
                    auto* entOverride = const_cast<YAMLNode*>(overrideNode)->GetChild(entityName);
                    if (entOverride)
                    {
                        LOGGER_INFO("FragmentLoader") << "Applying overrides to entity '" << entityName << "'";
                        YAMLNode::Merge(entNode, *entOverride);

                        // Debug merged children
                        for (auto& c : entNode.children)
                        {
                            if (c.key == "Component")
                            {
                                std::string subKeys = "";
                                for (auto& sc : c.children) subKeys += sc.key + ", ";
                                LOGGER_INFO("FragmentLoader")
                                    << " Merged component '" << c.value << "' keys: " << subKeys;
                            }
                        }
                    }
                }

                LoaderUtils::ValidateKeys(entNode, {"Tag", "Layer", "Parent", "Component"}, "Entity:" + entNode.key);
                std::string entityTag = entNode.GetChildValue("Tag", "default");

                std::string namespacedName = namePrefix + entityName;
                std::string namespacedTag = tagPrefix + entityTag;

                entt::entity currentEntity = scene.registry.create();
                instantiatedEntities[entityName] = currentEntity;  // KEEP LOCAL NAME FOR MAPPING

                scene.registry.emplace<PositionComponent>(currentEntity);
                scene.registry.emplace<RotationComponent>(currentEntity);
                scene.registry.emplace<ScaleComponent>(currentEntity);
                scene.registry.emplace<HierarchyComponent>(currentEntity);
                auto& wt = scene.registry.emplace<WorldTransformComponent>(currentEntity);
                wt.isDirty = true;
                scene.registry.emplace<InfoComponent>(currentEntity, namespacedName, namespacedTag);

                auto& info = scene.registry.get<InfoComponent>(currentEntity);
                info.sceneName = fragmentName;

                // Set parent to the provided parent by default
                if (parent != entt::null)
                {
                    if (!scene.registry.all_of<HierarchyComponent>(parent))
                    {
                        scene.registry.emplace<HierarchyComponent>(parent);
                    }
                    scene.registry.patch<HierarchyComponent>(currentEntity, [&](auto& h) { h.parent = parent; });
                    scene.registry.patch<HierarchyComponent>(parent,
                                                             [&](auto& h) { h.children.push_back(currentEntity); });
                }

                // Handle Transform component if present
                for (auto& child : entNode.children)
                {
                    if (child.key == "Component" && child.value == "Transform")
                    {
                        std::string pStr = child.GetChildValue("Position", "0 0 0");
                        std::string rStr = child.GetChildValue("Rotation", "0 0 0");
                        std::string sStr = child.GetChildValue("Scale", "1 1 1");
                        if (pStr.empty())
                            pStr = "0 0 0";
                        if (rStr.empty())
                            rStr = "0 0 0";
                        if (sStr.empty())
                            sStr = "1 1 1";

                        float x = 0, y = 0, z = 0;
                        std::stringstream ssP(pStr);
                        ssP >> x >> y >> z;

                        float rx = 0, ry = 0, rz = 0;
                        std::stringstream ssR(rStr);
                        ssR >> rx >> ry >> rz;

                        float sx = 1, sy = 1, sz = 1;
                        std::stringstream ssS(sStr);
                        ssS >> sx >> sy >> sz;

                        auto& pComp = scene.registry.get<PositionComponent>(currentEntity);
                        pComp.value = glm::vec3(x, y, z);
                        pComp.prev = pComp.value;

                        auto& rComp = scene.registry.get<RotationComponent>(currentEntity);
                        rComp.value = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
                        rComp.prev = rComp.value;

                        auto& sComp = scene.registry.get<ScaleComponent>(currentEntity);
                        sComp.value = glm::vec3(sx, sy, sz);
                        sComp.prev = sComp.value;

                        LOGGER_INFO("FragmentLoader")
                            << "[DEBUG] Loaded Transform for '" << entityName << "': Pos(" << x << "," << y << "," << z
                            << ") Scale(" << sx << "," << sy << "," << sz << ")";

                        // Mark dirty ΓÇö TransformSystem will compute the correct world matrix
                        // AFTER deferred internal parenting is resolved.
                        // Do NOT pre-compute here: `parent` is the fragment owner, not the
                        // internal parent (which hasn't been linked yet).
                        if (scene.registry.all_of<WorldTransformComponent>(currentEntity))
                        {
                            scene.registry.get<WorldTransformComponent>(currentEntity).isDirty = true;
                        }
                        break;
                    }
                }

                if (auto* pNode = entNode.GetChild("Parent"))
                {
                    deferredChildren[currentEntity].push_back(pNode->value);
                }

                for (auto& compNode : entNode.children)
                {
                    if (compNode.key != "Component")
                        continue;
                    std::string compType = compNode.value;
                    if (compType == "Transform")
                        continue;  // Already handled

                    ComponentLoader::Load(compType, scene, currentEntity, compNode, res, phys);
                }
            }
        }
    }

    // Resolve internal parenting within the fragment
    for (auto const& [entity, parents] : deferredChildren)
    {
        for (auto const& parentName : parents)
        {
            if (instantiatedEntities.find(parentName) != instantiatedEntities.end())
            {
                entt::entity internalParent = instantiatedEntities[parentName];

                // Remove from the default fragment parent if it was added
                if (parent != entt::null)
                {
                    scene.registry.patch<HierarchyComponent>(parent, [&](auto& ph) {
                        ph.children.erase(std::remove(ph.children.begin(), ph.children.end(), entity),
                                          ph.children.end());
                    });
                }

                scene.registry.patch<HierarchyComponent>(entity, [&](auto& h) { h.parent = internalParent; });
                scene.registry.patch<HierarchyComponent>(internalParent,
                                                         [&](auto& ph) { ph.children.push_back(entity); });
            }
        }
    }

    return instantiatedEntities;
}
