#include <editor/panels/scene_hierarchy_panel.h>
#include <editor/editor_system.h>

#ifdef ENABLE_EDITOR
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/logic/transform_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/network_components.h>
#include <ecs/unit/ui_components.h>
#include <engine/ecs/unit/fragment_component.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <script/logic/script_registry.h>
#include <script/logic/scriptable.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <utility>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace
{
void MarkTransformGraphDirty()
{
    if (auto* transformSystem = ServiceLocator::Instance().Resolve<TransformSystem>())
        transformSystem->MarkTransformGraphDirty();
}

void EnsureTransformComponents(entt::registry& reg, entt::entity entity)
{
    if (!reg.try_get<PositionComponent>(entity))
        reg.emplace<PositionComponent>(entity);
    if (!reg.try_get<RotationComponent>(entity))
        reg.emplace<RotationComponent>(entity);
    if (!reg.try_get<ScaleComponent>(entity))
        reg.emplace<ScaleComponent>(entity, glm::vec3(1.0f));

    auto& world = reg.get_or_emplace<WorldTransformComponent>(entity);
    world.isDirty = true;
    MarkTransformGraphDirty();
}

std::string SerializeYAMLNode(const YAMLNode& n, int indent)
{
    std::string res = std::string(indent * 2, ' ') + n.key;
    if (!n.value.empty())
        res += ": " + n.value;
    res += (n.value.empty() ? ":" : "") + std::string("\n");
    for (const auto& child : n.children)
    {
        res += SerializeYAMLNode(child, indent + 1);
    }
    return res;
}
}  // namespace

entt::entity SceneHierarchyPanel::s_SelectedEntity = entt::null;

void SceneHierarchyPanel::SetSelectedEntity(entt::entity entity)
{
    s_SelectedEntity = entity;
    EventManager::Instance().Publish(EntitySelectionChangedEvent{static_cast<uint32_t>(entity)});
}

void SceneHierarchyPanel::RequestFocus(entt::entity entity)
{
    EventManager::Instance().Publish(EntityFocusRequestedEvent{static_cast<uint32_t>(entity)});
}

void SceneHierarchyPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto& registry = scene.GetRegistry();
    if (s_SelectedEntity != entt::null && !registry.valid(s_SelectedEntity))
        SetSelectedEntity(entt::null);

    // Group entities by sceneName
    std::unordered_map<std::string, std::vector<entt::entity>> sceneGroups;

    // Only fetch Root entities
    for (auto entity : registry.view<InfoComponent>())
    {
        bool isRoot = true;
        if (registry.all_of<HierarchyComponent>(entity))
        {
            if (registry.get<HierarchyComponent>(entity).parent != entt::null)
            {
                isRoot = false;
            }
        }
        if (isRoot)
        {
            std::string sname = "Default Scene";
            std::string sn = registry.get<InfoComponent>(entity).sceneName;
            if (!sn.empty())
                sname = sn;
            sceneGroups[sname].push_back(entity);
        }
    }

    // Search filter
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##EntitySearch", "Search entities...", m_SearchFilter, IM_ARRAYSIZE(m_SearchFilter));
    std::string filterLower = m_SearchFilter;
    std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

    // Ctrl+D duplicate shortcut
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        auto& kb = io->GetKeyboard();
        bool ctrl = kb.GetKey(Key::LeftControl) || kb.GetKey(Key::RightControl);
        if (ctrl && kb.GetKey(Key::D) && !m_CtrlDPressed && s_SelectedEntity != entt::null &&
            registry.valid(s_SelectedEntity))
        {
            m_CtrlDPressed = true;
            EditorSystem::PushUndoState(scene);
            DuplicateEntity(scene, s_SelectedEntity);
        }
        if (!kb.GetKey(Key::D))
            m_CtrlDPressed = false;

        // Delete (Delete)
        static bool deletePressed = false;
        if (kb.GetKey(Key::Delete) && !deletePressed && s_SelectedEntity != entt::null &&
            registry.valid(s_SelectedEntity))
        {
            deletePressed = true;
            EditorSystem::PushUndoState(scene);
            auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>();
            scene.DestroyEntityWithChildren(s_SelectedEntity, sceneMgr);
            SetSelectedEntity(entt::null);
            MarkTransformGraphDirty();
        }
        if (!kb.GetKey(Key::Delete))
            deletePressed = false;
    }

    ImGui::Columns(2, "HierarchyInspector", true);
    ImGui::BeginChild("HierarchyTree");

    auto& sm = ServiceLocator::Instance().Require<SceneManager>();
    std::string activeScene = sm.GetActiveScene();

    for (const auto& [sname, entities] : sceneGroups)
    {
        ImGui::PushID(sname.c_str());
        auto* rec = sm.GetSceneByName(sname);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (rec && rec->isActive)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)sname.length(), flags, sname.c_str());
        if (rec && rec->isActive)
            ImGui::PopStyleColor();

        if (ImGui::BeginPopupContextItem())
        {
            if (rec)
            {
                if (ImGui::MenuItem(rec->isActive ? "Deactivate Scene" : "Activate Scene"))
                {
                    sm.SetSceneActive(sname, !rec->isActive, scene);
                }
                ImGui::Separator();
            }
            if (ImGui::MenuItem("Save Scene"))
            {
                auto* res = ServiceLocator::Instance().Resolve<ResourceManager>();
                if (rec && res)
                {
                    SceneSerializer::Serialize(rec->filePath, scene, *res, sname);
                }
            }
            if (ImGui::MenuItem("Add Entity"))
            {
                CreateNewEntity(scene, sname);
            }
            ImGui::EndPopup();
        }

        if (isOpen)
        {
            for (auto entity : entities)
            {
                // Apply search filter
                if (!filterLower.empty())
                {
                    std::string ename = "";
                    if (registry.all_of<InfoComponent>(entity))
                    {
                        ename = registry.get<InfoComponent>(entity).name;
                    }
                    std::string enameLower = ename;
                    std::transform(enameLower.begin(), enameLower.end(), enameLower.begin(), ::tolower);
                    if (enameLower.find(filterLower) == std::string::npos)
                        continue;
                }
                DrawEntityNode(scene, entity);
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("InspectorView");
    if (s_SelectedEntity != entt::null && registry.valid(s_SelectedEntity))
    {
        DrawComponents(registry, s_SelectedEntity);
    }
    else
    {
        ImGui::Text("No Entity Selected");
    }
    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Scene& scene, entt::entity entity)
{
    auto& registry = scene.GetRegistry();
    if (!registry.valid(entity))
        return;

    uint32_t entityId = static_cast<uint32_t>(entity);
    std::string name = "Entity " + std::to_string(entityId);

    if (registry.all_of<InfoComponent>(entity))
    {
        name = registry.get<InfoComponent>(entity).name;
    }

    bool hasChildren = false;
    const std::vector<entt::entity>* children = nullptr;

    if (registry.all_of<HierarchyComponent>(entity))
    {
        children = &registry.get<HierarchyComponent>(entity).children;
        hasChildren = !children->empty();
    }

    ImGuiTreeNodeFlags flags =
        ((s_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool isFragment = registry.all_of<FragmentComponent>(entity);
    bool hasOverride = false;
    if (isFragment)
    {
        hasOverride = !registry.get<FragmentComponent>(entity).overrides.empty();
    }

    if (hasOverride)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entityId, flags, "%s", name.c_str());
    if (hasOverride)
        ImGui::PopStyleColor();
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        SetSelectedEntity(entity);
        RequestFocus(entity);
    }
    if (ImGui::IsItemClicked())
    {
        SetSelectedEntity(entity);
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
            entityDeleted = true;
        if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D"))
        {
            EditorSystem::PushUndoState(scene);
            DuplicateEntity(scene, entity);
        }
        ImGui::EndPopup();
    }

    if (opened && hasChildren && children)
    {
        auto childrenSnapshot = *children;
        for (auto child : childrenSnapshot)
        {
            if (registry.valid(child))
                DrawEntityNode(scene, child);
        }
        ImGui::TreePop();
    }

    if (entityDeleted)
    {
        EditorSystem::PushUndoState(scene);
        if (s_SelectedEntity == entity)
            SetSelectedEntity(entt::null);
        auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>();
        scene.DestroyEntityWithChildren(entity, sceneMgr);
        MarkTransformGraphDirty();
    }
}

template <typename T, typename UIFunction>
static void DrawComponent(const std::string& name, entt::registry& reg, entt::entity entity, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                             ImGuiTreeNodeFlags_FramePadding;

    if (auto* component = reg.try_get<T>(entity))
    {
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - lineHeight - 4.0f);
        ImGui::PushID((void*)typeid(T).hash_code());
        if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;

            ImGui::EndPopup();
        }
        ImGui::PopID();

        if (open)
        {
            uiFunction(*component);
            ImGui::TreePop();
        }

        if (removeComponent)
        {
            auto& globalScene = ServiceLocator::Instance().Require<Scene>();
            EditorSystem::PushUndoState(globalScene);
            reg.remove<T>(entity);
        }
    }
}

static bool DrawResourceDropdownStr(const char* label, std::string& currentName,
                                    const std::vector<std::string>& resources)
{
    bool changed = false;
    std::string displayName = currentName.empty() ? "None" : currentName;
    if (ImGui::BeginCombo(label, displayName.c_str()))
    {
        static char searchBuf[256] = "";

        // Fixed area for Search and Sort
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", searchBuf, IM_ARRAYSIZE(searchBuf));

        static int sortMode = 0;  // 0: Default, 1: A-Z, 2: Z-A
        ImGui::RadioButton("Default", &sortMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("A-Z", &sortMode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Z-A", &sortMode, 2);

        ImGui::Separator();

        // Scrollable area for results
        if (ImGui::BeginChild("##DropdownScroll", ImVec2(0, 200), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            std::vector<std::string> sortedResources = resources;
            if (sortMode == 1)
            {
                std::sort(sortedResources.begin(), sortedResources.end());
            }
            else if (sortMode == 2)
            {
                std::sort(sortedResources.begin(), sortedResources.end(), std::greater<std::string>());
            }

            std::string searchStr = searchBuf;
            std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

            if (searchStr.empty() || std::string("none").find(searchStr) != std::string::npos)
            {
                bool isSelected = currentName.empty();
                if (ImGui::Selectable("None", isSelected))
                {
                    currentName = "";
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            for (const auto& resName : sortedResources)
            {
                std::string lowerName = resName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (!searchStr.empty() && lowerName.find(searchStr) == std::string::npos)
                    continue;

                bool isSelected = (currentName == resName);
                if (ImGui::Selectable(resName.c_str(), isSelected))
                {
                    currentName = resName;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndChild();
        }
        ImGui::EndCombo();
    }
    return changed;
}

template <typename T>
static bool DrawResourceDropdown(const char* label, std::shared_ptr<T>& currentResource,
                                 const std::vector<std::string>& resourceNames,
                                 std::function<std::shared_ptr<T>(const std::string&)> getter)
{
    std::string currentName = currentResource ? currentResource->GetName() : "";
    if (DrawResourceDropdownStr(label, currentName, resourceNames))
    {
        currentResource = currentName.empty() ? nullptr : getter(currentName);
        return true;
    }
    return false;
}

void SceneHierarchyPanel::DrawComponents(entt::registry& reg, entt::entity entity)
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    if (auto* info = reg.try_get<InfoComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char buffer[256];
            strncpy(buffer, info->name.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("Name", buffer, sizeof(buffer)))
                info->name = buffer;
            ImGui::Text("Scene: %s", info->sceneName.c_str());
            ImGui::Text("Tag: %s", info->tag.c_str());
        }
    }

    if (auto* pos = reg.try_get<PositionComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool transformChanged = false;
            if (ImGui::DragFloat3("Position", &pos->value.x, 0.1f))
            {
                pos->prev = pos->value;
                transformChanged = true;
            }
            if (auto* rot = reg.try_get<RotationComponent>(entity))
            {
                glm::vec3 euler = glm::degrees(glm::eulerAngles(rot->value));
                if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f))
                {
                    rot->value = glm::quat(glm::radians(euler));
                    rot->prev = rot->value;
                    transformChanged = true;
                }
            }
            if (auto* scale = reg.try_get<ScaleComponent>(entity))
            {
                if (ImGui::DragFloat3("Scale", &scale->value.x, 0.01f, 0.001f, 100.0f))
                {
                    scale->prev = scale->value;
                    transformChanged = true;
                }
            }
            // Mark cached world transform as dirty so transform system rebuilds it
            if (transformChanged)
            {
                if (auto* wt = reg.try_get<WorldTransformComponent>(entity))
                {
                    wt->isDirty = true;
                }
            }
        }
    }

    DrawComponent<CameraComponent>("Camera", reg, entity, [](auto& cam) {
        ImGui::Checkbox("Primary", &cam.isPrimary);
        ImGui::DragFloat("FOV", &cam.fov, 0.1f);
        ImGui::DragFloat("Near Clip", &cam.nearPlane, 0.1f);
        ImGui::DragFloat("Far Clip", &cam.farPlane, 0.1f);
    });

    DrawComponent<MeshRendererComponent>("Mesh Renderer", reg, entity, [](auto& mesh) {
        ImGui::ColorEdit4("Color", &mesh.color.r);
        ImGui::Checkbox("Cast Shadow", &mesh.castShadow);
        ImGui::Checkbox("Receive Shadow", &mesh.receiveShadow);
        ImGui::Checkbox("Ignore Depth", &mesh.ignoreDepth);

        static const char* renderModes[] = {"Auto", "Force Forward"};
        int currentMode = (mesh.renderMode == RenderMode::ForceForward) ? 1 : 0;
        if (ImGui::Combo("Render Mode", &currentMode, renderModes, IM_ARRAYSIZE(renderModes)))
        {
            mesh.renderMode = (currentMode == 1) ? RenderMode::ForceForward : RenderMode::Auto;
        }

        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto modelNames = rm.GetLoadedModels();
        auto shaderNames = rm.GetLoadedShaders();

        DrawResourceDropdown<Model>("Model", mesh.model, modelNames,
                                    [&rm](const std::string& name) { return rm.GetModel(name); });

        std::shared_ptr<Shader> currentShader = mesh.shader.lock();
        if (DrawResourceDropdown<Shader>("Shader", currentShader, shaderNames,
                                         [&rm](const std::string& name) { return rm.GetShader(name); }))
        {
            mesh.shader = currentShader;
            mesh.shaderName = currentShader ? currentShader->GetName() : "";
        }

        ImGui::DragInt("Render Order", &mesh.order, 1, 0, 255);
    });

    DrawComponent<DirectionalLightComponent>("Directional Light", reg, entity, [](auto& light) {
        ImGui::Checkbox("Active", &light.active);
        ImGui::DragFloat3("Direction", &light.direction.x, 0.1f);
        ImGui::ColorEdit3("Color", &light.color.r);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f);
        ImGui::DragFloat("Ambient", &light.ambient, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Diffuse", &light.diffuse, 0.01f, 0.0f, 4.0f);
        ImGui::DragFloat("Specular", &light.specular, 0.01f, 0.0f, 4.0f);
        ImGui::Checkbox("Cast Shadow", &light.isCastShadow);
    });

    DrawComponent<PointLightComponent>("Point Light", reg, entity, [](auto& light) {
        ImGui::Checkbox("Active", &light.active);
        ImGui::ColorEdit3("Color", &light.color.r);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f);
        ImGui::DragFloat("Radius", &light.radius, 0.5f);
        ImGui::DragFloat("Ambient", &light.ambient, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Diffuse", &light.diffuse, 0.01f, 0.0f, 4.0f);
        ImGui::DragFloat("Specular", &light.specular, 0.01f, 0.0f, 4.0f);
        ImGui::Checkbox("Cast Shadow", &light.isCastShadow);
    });

    DrawComponent<SpotLightComponent>("Spot Light", reg, entity, [](auto& light) {
        ImGui::Checkbox("Active", &light.active);
        ImGui::DragFloat3("Direction", &light.direction.x, 0.1f);
        ImGui::ColorEdit3("Color", &light.color.r);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f);
        ImGui::DragFloat("Radius", &light.radius, 0.5f);
        ImGui::DragFloat("Ambient", &light.ambient, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Diffuse", &light.diffuse, 0.01f, 0.0f, 4.0f);
        ImGui::DragFloat("Specular", &light.specular, 0.01f, 0.0f, 4.0f);
        ImGui::Checkbox("Cast Shadow", &light.isCastShadow);
    });

    DrawComponent<RigidShapeComponent>("Rigid Shape", reg, entity, [&reg, entity](auto& shape) {
        bool shapeChanged = false;
        static const char* shapeTypes[] = {"Box", "Sphere", "Capsule", "Cylinder", "Mesh", "Heightfield", "Compound"};
        int currentType = (int)shape.type;
        if (ImGui::Combo("Shape Type", &currentType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
        {
            shape.type = (ShapeType)currentType;
            shapeChanged = true;
        }
        if (shape.type == ShapeType::Box)
            shapeChanged |= ImGui::DragFloat3("Size", &shape.size.x, 0.1f);
        if (shape.type == ShapeType::Sphere || shape.type == ShapeType::Capsule || shape.type == ShapeType::Cylinder)
            shapeChanged |= ImGui::DragFloat("Radius", &shape.radius, 0.1f);
        if (shape.type == ShapeType::Capsule || shape.type == ShapeType::Cylinder)
            shapeChanged |= ImGui::DragFloat("Height", &shape.height, 0.1f);

        shapeChanged |= ImGui::DragFloat("Friction", &shape.friction, 0.05f, 0.0f, 1.0f);
        shapeChanged |= ImGui::DragFloat("Restitution", &shape.restitution, 0.05f, 0.0f, 1.0f);
        shapeChanged |= ImGui::DragFloat3("Offset", &shape.offset.x, 0.1f);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(shape.rotation));
        if (ImGui::DragFloat3("Rotation Offset", &euler.x, 0.5f))
        {
            shape.rotation = glm::quat(glm::radians(euler));
            shapeChanged = true;
        }

        if (shape.type == ShapeType::Compound)
        {
            ImGui::Separator();
            ImGui::Text("Child Shapes (%d)", (int)shape.children.size());
            if (ImGui::Button("Add Child Shape"))
            {
                shape.children.push_back(
                    {ShapeType::Box, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.5f, 1.0f});
                shapeChanged = true;
            }

            for (size_t i = 0; i < shape.children.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));

                if (ImGui::TreeNode("Child", "Child %d", (int)i))
                {
                    auto& child = shape.children[i];
                    int cType = (int)child.type;
                    if (ImGui::Combo("Type", &cType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
                    {
                        child.type = (ShapeType)cType;
                        shapeChanged = true;
                    }

                    if (child.type == ShapeType::Box)
                        shapeChanged |= ImGui::DragFloat3("Size", &child.size.x, 0.1f);
                    if (child.type == ShapeType::Sphere || child.type == ShapeType::Capsule ||
                        child.type == ShapeType::Cylinder)
                        shapeChanged |= ImGui::DragFloat("Radius", &child.radius, 0.1f);
                    if (child.type == ShapeType::Capsule || child.type == ShapeType::Cylinder)
                        shapeChanged |= ImGui::DragFloat("Height", &child.height, 0.1f);

                    shapeChanged |= ImGui::DragFloat3("Position", &child.position.x, 0.1f);

                    glm::vec3 cEuler = glm::degrees(glm::eulerAngles(child.rotation));
                    if (ImGui::DragFloat3("Rotation", &cEuler.x, 0.5f))
                    {
                        child.rotation = glm::quat(glm::radians(cEuler));
                        shapeChanged = true;
                    }

                    if (ImGui::Button("Remove"))
                    {
                        shape.children.erase(shape.children.begin() + i);
                        shapeChanged = true;
                        ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::Separator();
        }

        if (shapeChanged && reg.all_of<RigidBodyComponent>(entity))
        {
            auto rbCopy = reg.get<RigidBodyComponent>(entity);
            reg.erase<RigidBodyComponent>(entity);
            rbCopy.body = nullptr;
            reg.emplace<RigidBodyComponent>(entity, rbCopy);
        }
    });

    DrawComponent<RigidBodyComponent>("Rigid Body", reg, entity, [&reg, entity](auto& rb) {
        bool rbChanged = false;

        int bodyType = 2;  // Dynamic
        if (rb.isStatic)
            bodyType = 0;
        else if (rb.isKinematic)
            bodyType = 1;

        const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
        if (ImGui::Combo("Body Type", &bodyType, bodyTypes, IM_ARRAYSIZE(bodyTypes)))
        {
            rb.isStatic = (bodyType == 0);
            rb.isKinematic = (bodyType == 1);
            rbChanged = true;
        }

        rbChanged |= ImGui::DragFloat("Mass", &rb.mass, 0.1f, 0.0f, 10000.0f);

        bool canCollide = !rb.isTrigger;
        if (ImGui::Checkbox("Can Collide", &canCollide))
        {
            rb.isTrigger = !canCollide;
            rbChanged = true;
        }

        ImGui::Text("Lock Position:");
        ImGui::SameLine();
        bool lockX = rb.linearFactor.x < 0.5f;
        bool lockY = rb.linearFactor.y < 0.5f;
        bool lockZ = rb.linearFactor.z < 0.5f;
        if (ImGui::Checkbox("X##L", &lockX))
        {
            rb.linearFactor.x = lockX ? 0.0f : 1.0f;
            rbChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Y##L", &lockY))
        {
            rb.linearFactor.y = lockY ? 0.0f : 1.0f;
            rbChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Z##L", &lockZ))
        {
            rb.linearFactor.z = lockZ ? 0.0f : 1.0f;
            rbChanged = true;
        }

        ImGui::Text("Lock Rotation:");
        ImGui::SameLine();
        bool lockRX = rb.angularFactor.x < 0.5f;
        bool lockRY = rb.angularFactor.y < 0.5f;
        bool lockRZ = rb.angularFactor.z < 0.5f;
        if (ImGui::Checkbox("X##R", &lockRX))
        {
            rb.angularFactor.x = lockRX ? 0.0f : 1.0f;
            rbChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Y##R", &lockRY))
        {
            rb.angularFactor.y = lockRY ? 0.0f : 1.0f;
            rbChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Z##R", &lockRZ))
        {
            rb.angularFactor.z = lockRZ ? 0.0f : 1.0f;
            rbChanged = true;
        }

        rbChanged |= ImGui::DragFloat("Linear Damping", &rb.linearDamping, 0.01f, 0.0f, 1.0f);
        rbChanged |= ImGui::DragFloat("Angular Damping", &rb.angularDamping, 0.01f, 0.0f, 1.0f);

        if (rbChanged)
        {
            auto rbCopy = rb;
            reg.erase<RigidBodyComponent>(entity);
            rbCopy.body = nullptr;
            reg.emplace<RigidBodyComponent>(entity, rbCopy);
        }
    });

    DrawComponent<CharacterControllerComponent>("Character Controller", reg, entity, [](auto& cc) {
        ImGui::DragFloat("Step Height", &cc.stepHeight, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Max Slope", &cc.maxSlope, 0.5f, 0.0f, 90.0f);
        ImGui::Text("On Ground: %s", cc.isOnGround ? "Yes" : "No");
    });

    DrawComponent<MaterialComponent>("Material", reg, entity, [](auto& mat) {
        bool changed = false;
        changed |= ImGui::DragFloat("Opacity", &mat.desc.opacity, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::ColorEdit3("Emission", &mat.desc.emission.r);
        changed |= ImGui::DragFloat2("UV Scale", &mat.desc.uvScale.x, 0.01f);
        changed |= ImGui::DragFloat2("UV Offset", &mat.desc.uvOffset.x, 0.01f);

        ImGui::Separator();
        changed |= ImGui::DragFloat("Roughness", &mat.desc.pbr.roughness, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("Metallic", &mat.desc.pbr.metallic, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("AO", &mat.desc.pbr.ao, 0.01f, 0.0f, 1.0f);

        if (changed)
            mat.gpu.batchKeyDirty = true;
    });

    DrawComponent<UITransformComponent>("UI Transform", reg, entity, [](auto& uiTransform) {
        auto DrawVec2Percent = [](const char* label, glm::vec2& v, glm::bvec2& p, float speed) {
            ImGui::PushID(label);
            ImGui::DragFloat2(label, &v.x, speed);
            ImGui::SameLine();
            ImGui::Checkbox("X%", &p.x);
            ImGui::SameLine();
            ImGui::Checkbox("Y%", &p.y);
            ImGui::PopID();
        };

        DrawVec2Percent("Position##UI", uiTransform.position, uiTransform.positionIsPercent, 1.0f);
        DrawVec2Percent("Size##UI", uiTransform.size, uiTransform.sizeIsPercent, 1.0f);

        ImGui::DragFloat("Rotation##UI", &uiTransform.rotation, 1.0f);
        ImGui::DragInt("Z-Index##UI", &uiTransform.zIndex);
        ImGui::DragFloat2("Pivot##UI", &uiTransform.pivot.x, 0.05f);
        ImGui::Checkbox("Flip X##UI", &uiTransform.flipX);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y##UI", &uiTransform.flipY);

        DrawVec2Percent("Anchor Min##UI", uiTransform.anchorMin, uiTransform.anchorMinIsPercent, 0.05f);
        DrawVec2Percent("Anchor Max##UI", uiTransform.anchorMax, uiTransform.anchorMaxIsPercent, 0.05f);
        DrawVec2Percent("Offset Min##UI", uiTransform.offsetMin, uiTransform.offsetMinIsPercent, 1.0f);
        DrawVec2Percent("Offset Max##UI", uiTransform.offsetMax, uiTransform.offsetMaxIsPercent, 1.0f);
    });

    DrawComponent<UIFlexLayoutComponent>("UI Flex Layout", reg, entity, [](auto& uiFlex) {
        static const char* flexDirs[] = {"Row", "Column"};
        int currentDir = (int)uiFlex.direction;
        if (ImGui::Combo("Direction##UI", &currentDir, flexDirs, IM_ARRAYSIZE(flexDirs)))
        {
            uiFlex.direction = (FlexDirection)currentDir;
        }
        ImGui::DragFloat("Spacing##UI", &uiFlex.spacing, 1.0f);
        ImGui::Checkbox("Auto Size##UI", &uiFlex.autoSize);
        ImGui::DragFloat4("Padding##UI", &uiFlex.padding.x, 1.0f);
    });

    DrawComponent<UIRendererComponent>("UI Renderer", reg, entity, [](auto& uiRenderer) {
        ImGui::ColorEdit4("Color##UI", &uiRenderer.color.r);
        ImGui::Text("Model: %s", uiRenderer.model ? "Loaded" : "None");
        ImGui::Text("Shader: %s", uiRenderer.shader ? "Loaded" : "None");
        ImGui::Text("Texture: %s", uiRenderer.texture ? "Loaded" : "None");
    });

    DrawComponent<UITextComponent>("UI Text", reg, entity, [](auto& uiText) {
        char buffer[256];
        strncpy(buffer, uiText.text.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';
        if (ImGui::InputText("Text##UI", buffer, sizeof(buffer)))
        {
            uiText.text = buffer;
        }
        ImGui::ColorEdit4("Color##UIText", &uiText.color.r);
        ImGui::DragFloat("Scale##UIText", &uiText.scale, 0.01f);

        static const char* alignments[] = {"Left", "Center", "Right"};
        int currentAlign = (int)uiText.alignment;
        if (ImGui::Combo("Alignment##UI", &currentAlign, alignments, IM_ARRAYSIZE(alignments)))
        {
            uiText.alignment = (TextAlignment)currentAlign;
        }

        ImGui::Checkbox("Word Wrap##UI", &uiText.wordWrap);
        if (uiText.wordWrap)
        {
            ImGui::DragFloat("Max Width##UI", &uiText.maxWidth, 1.0f);
        }
        ImGui::Text("Font: %s", uiText.fontName.empty() ? "None" : uiText.fontName.c_str());
    });

    DrawComponent<ScriptComponent>("Script", reg, entity, [&reg, entity](auto& script) {
        std::vector<std::string> scriptNames;
        for (const auto& pair : ScriptRegistry::GetStaticFactoryMap())
        {
            scriptNames.push_back(pair.first);
        }

        std::string currentScript = script.className;
        if (DrawResourceDropdownStr("Script Class", currentScript, scriptNames))
        {
            if (script.instance && script.DestroyScript)
            {
                script.DestroyScript(&script);
                script.instance.reset();
                script.scriptableInstance = nullptr;
                script.inputScriptableInstance = nullptr;
            }

            script.className = currentScript;
            if (currentScript.empty() || currentScript == "None")
            {
                script.InstantiateScript = nullptr;
                script.DestroyScript = nullptr;
            }
            else
            {
                auto& factoryMap = ScriptRegistry::GetStaticFactoryMap();
                if (factoryMap.count(currentScript))
                {
                    auto factory = factoryMap.at(currentScript);
                    script.InstantiateScript = factory;
                    script.DestroyScript = [](ScriptComponent* sc) {
                        sc->instance.reset();
                        sc->scriptableInstance = nullptr;
                        sc->inputScriptableInstance = nullptr;
                    };
                    script.instance = script.InstantiateScript();

                    auto& globalScene = ServiceLocator::Instance().Require<Scene>();
                    script.instance->Initialize(entity, &globalScene);
                    script.instance->OnCreate();
                }
            }
        }
    });

    DrawComponent<ReflectiveComponent>("Reflective", reg, entity, [&reg](auto& ref) {
        ImGui::Checkbox("Enabled##Ref", &ref.enabled);
        ImGui::DragFloat("Reflectivity", &ref.reflectivity, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Fresnel Power", &ref.fresnelPower, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Fresnel Bias", &ref.fresnelBias, 0.01f, 0.0f, 1.0f);

        std::vector<std::string> probeNames;
        auto view = reg.view<InfoComponent, ReflectionProbeComponent>();
        for (auto e : view)
        {
            probeNames.push_back(view.get<InfoComponent>(e).name);
        }
        DrawResourceDropdownStr("Target Probe", ref.targetProbe, probeNames);
    });

    DrawComponent<ReflectionProbeComponent>("Reflection Probe", reg, entity, [](auto& probe) {
        const char* probeTypes[] = {"Static", "Dynamic"};
        int pt = (int)probe.type;
        if (ImGui::Combo("Probe Type", &pt, probeTypes, 2))
            probe.type = (ReflectionProbeType)pt;
        if (ImGui::DragInt("Resolution##Probe", &probe.resolution, 64, 64, 2048))
            probe.isDirty = true;
        ImGui::Checkbox("Box Projection", &probe.boxProjection);
        ImGui::DragFloat3("Box Min", &probe.boxMin.x, 0.1f);
        ImGui::DragFloat3("Box Max", &probe.boxMax.x, 0.1f);
        ImGui::DragFloat("Blend Distance", &probe.blendDistance, 0.1f, 0.0f, 50.0f);
        if (ImGui::Button("Refresh Probe"))
            probe.isDirty = true;
        ImGui::Text("Cubemap ID: %u", probe.cubemapID);
    });

    DrawComponent<AudioSourceComponent>("Audio Source", reg, entity, [](auto& audio) {
        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto soundNames = rm.GetLoadedSounds();

        std::shared_ptr<IAudioSource> currentSound = audio.source;
        if (DrawResourceDropdown<IAudioSource>("Sound", currentSound, soundNames,
                                               [&rm](const std::string& name) { return rm.GetSound(name); }))
        {
            audio.source = currentSound;
            audio.resourceName = currentSound ? currentSound->GetName() : "";
        }

        ImGui::Checkbox("Play On Awake", &audio.playOnAwake);
        ImGui::Checkbox("Loop##Audio", &audio.loop);
        ImGui::Checkbox("3D##Audio", &audio.is3D);
        ImGui::DragFloat("Volume##Audio", &audio.volume, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Pitch##Audio", &audio.pitch, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Pan##Audio", &audio.pan, 0.01f, -1.0f, 1.0f);
        if (audio.is3D)
        {
            ImGui::DragFloat("Min Distance", &audio.minDistance, 0.1f);
            ImGui::DragFloat("Max Distance", &audio.maxDistance, 0.1f);
        }

        if (audio.sound)
        {
            if (ImGui::Button("Play"))
                audio.shouldPlay = true;
            ImGui::SameLine();
            if (ImGui::Button("Pause"))
                audio.sound->Pause();
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
                audio.sound->Stop();
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No active ISound source");
        }
    });

    DrawComponent<VideoPlayerComponent>("Video Player", reg, entity, [](auto& video) {
        ImGui::Text("Path: %s", video.filePath.empty() ? "None" : video.filePath.c_str());
        ImGui::Checkbox("Loop##Video", &video.isLooping);
        ImGui::Checkbox("Play On Awake##Vid", &video.playOnAwake);
        ImGui::DragFloat("Volume##Video", &video.volume, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Speed##Video", &video.speed, 0.01f, 0.1f, 4.0f);
    });

    DrawComponent<ParticleEmitterComponent>("Particle Emitter", reg, entity, [](auto& pe) {
        ImGui::Checkbox("Active##Part", &pe.isActive);
        ImGui::DragFloat("Spawn Rate", &pe.emitter.SpawnRate, 0.5f, 0.0f, 1000.0f);
        ImGui::DragFloat("Lifetime##Part", &pe.emitter.LifeTime, 0.1f, 0.01f, 60.0f);
        ImGui::DragFloat("Start Size", &pe.emitter.StartSize, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("End Size", &pe.emitter.EndSize, 0.01f, 0.0f, 100.0f);
        ImGui::ColorEdit4("Start Color", &pe.emitter.StartColor.r);
        ImGui::ColorEdit4("End Color", &pe.emitter.EndColor.r);
        ImGui::DragFloat3("Min Velocity", &pe.emitter.MinVelocity.x, 0.1f);
        ImGui::DragFloat3("Max Velocity", &pe.emitter.MaxVelocity.x, 0.1f);

        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto textureNames = rm.GetLoadedTextures();
        if (DrawResourceDropdownStr("Texture", pe.textureName, textureNames))
        {
            pe.emitter.Texture = rm.GetTexture(pe.textureName);
        }
    });

    DrawComponent<PostProcessComponent>("Post Process", reg, entity, [](auto& pp) {
        ImGui::Checkbox("Enabled##PP", &pp.enabled);
        ImGui::Separator();
        ImGui::Text("Effects: %d", (int)pp.effects.size());

        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto shaderNames = rm.GetLoadedShaders();

        static std::string newEffectShader = "";
        DrawResourceDropdownStr("New Effect", newEffectShader, shaderNames);
        if (ImGui::Button("Add Effect") && !newEffectShader.empty())
        {
            PostProcessComponent::Effect newEff;
            newEff.shaderName = newEffectShader;
            newEff.priority = 100;
            pp.effects.push_back(newEff);
            newEffectShader = "";
        }

        for (size_t i = 0; i < pp.effects.size(); ++i)
        {
            auto& eff = pp.effects[i];
            ImGui::PushID((int)i);
            if (ImGui::TreeNode(eff.shaderName.c_str()))
            {
                ImGui::Checkbox("Enabled", &eff.enabled);
                ImGui::Checkbox("Affect UI", &eff.affectUI);

                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
                ImGui::DragInt("##X", &eff.x);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("X (Drag)");
                ImGui::SameLine();
                ImGui::DragInt("##Y", &eff.y);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Y (Drag)");
                ImGui::SameLine();
                ImGui::DragInt("##W", &eff.w);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("W (Drag)");
                ImGui::SameLine();
                ImGui::DragInt("##H", &eff.h);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("H (Drag)");
                ImGui::PopItemWidth();

                if (ImGui::Button("Move Up") && i > 0)
                {
                    std::swap(pp.effects[i], pp.effects[i - 1]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Move Down") && i < pp.effects.size() - 1)
                {
                    std::swap(pp.effects[i], pp.effects[i + 1]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    pp.effects.erase(pp.effects.begin() + i);
                    ImGui::TreePop();
                    ImGui::PopID();
                    break;
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    });

    DrawComponent<AnimationComponent>("Animator", reg, entity, [](auto& anim) {
        if (ImGui::Button("Add Clip"))
            anim.animations.push_back("new_animation");

        for (int i = 0; i < (int)anim.animations.size(); ++i)
        {
            char buf[128];
            strncpy(buf, anim.animations[i].c_str(), sizeof(buf));
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 30);
            if (ImGui::InputText(("##Clip" + std::to_string(i)).c_str(), buf, sizeof(buf)))
            {
                anim.animations[i] = buf;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(("X##" + std::to_string(i)).c_str()))
            {
                anim.animations.erase(anim.animations.begin() + i);
                i--;
            }
        }

        ImGui::Separator();
        ImGui::DragFloat("Speed", &anim.speed, 0.05f, 0.0f, 10.0f);
        ImGui::DragFloat("Start Time", &anim.startTime, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Rate", &anim.rate, 1.0f, 0.0f, 240.0f);
        ImGui::DragFloat("Blend Factor", &anim.blendFactor, 0.01f, 0.0f, 1.0f);

        if (anim.animator)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Running");
            ImGui::Text("Current Time: %.2f", anim.animator->GetCurrentTime());

            static int selectedPreview = 0;
            if (selectedPreview >= (int)anim.animations.size())
                selectedPreview = 0;
            if (!anim.animations.empty())
            {
                std::vector<const char*> clips;
                for (const auto& s : anim.animations) clips.push_back(s.c_str());
                ImGui::Combo("Preview Clip", &selectedPreview, clips.data(), (int)clips.size());
                if (ImGui::Button("Play Preview"))
                    anim.animator->PlayAnimation(anim.animations[selectedPreview]);
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: No Animator (Check Model)");
        }
    });

    DrawComponent<DecalComponent>("Decal", reg, entity, [](auto& decal) {
        ImGui::DragFloat("Opacity##Decal", &decal.opacity, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Roughness##Decal", &decal.roughness, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Metallic##Decal", &decal.metallic, 0.01f, 0.0f, 1.0f);
        ImGui::ColorEdit4("Tint##Decal", &decal.tintColor.r);
        ImGui::DragFloat("Lifetime##Decal", &decal.lifetime, 0.1f);
        int ro = (int)decal.renderOrder;
        if (ImGui::DragInt("Render Order##Decal", &ro, 1, 0, 255))
            decal.renderOrder = (uint32_t)ro;
        const char* lightModes[] = {"None", "Light", "Light+Shadow"};
        int lm = (int)decal.lightingMode;
        if (ImGui::Combo("Lighting##Decal", &lm, lightModes, 3))
            decal.lightingMode = lm;
    });

    DrawComponent<TerrainComponent>("Terrain", reg, entity, [](auto& terrain) {
        ImGui::DragFloat3("Terrain Size", &terrain.terrainSize.x, 1.0f);
        ImGui::DragInt("Resolution##Terrain", &terrain.resolution, 1, 16, 4096);
        ImGui::DragFloat("Texture Scale", &terrain.textureScale, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Max Height", &terrain.maxHeight, 0.1f, 0.0f, 500.0f);
        ImGui::Text("Diffuse Layers: %d", (int)terrain.diffuseLayers.size());
        ImGui::Checkbox("Cast Shadows##Terr", &terrain.castShadows);
        ImGui::Checkbox("Generate Physics", &terrain.generatePhysics);
    });

    DrawComponent<LODComponent>("LOD", reg, entity, [](auto& lod) {
        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto modelNames = rm.GetLoadedModels();

        const size_t pairCount = std::max(lod.lodModels.size(), lod.lodDistancesSq.size());
        while (lod.lodModels.size() < pairCount) lod.lodModels.push_back(nullptr);
        while (lod.lodDistancesSq.size() < pairCount)
        {
            float previousDistance = lod.lodDistancesSq.empty() ? 0.0f : std::sqrt(lod.lodDistancesSq.back());
            float nextDistance = previousDistance + 10.0f;
            lod.lodDistancesSq.push_back(nextDistance * nextDistance);
        }

        ImGui::Text("LOD Pairs: %d", (int)lod.lodModels.size());
        ImGui::Separator();

        int removeIndex = -1;
        for (size_t i = 0; i < lod.lodModels.size(); ++i)
        {
            ImGui::PushID((int)i);
            ImGui::Text("L%d", (int)i);
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
                removeIndex = (int)i;

            std::shared_ptr<Model> currentModel = lod.lodModels[i];
            if (DrawResourceDropdown<Model>("Model", currentModel, modelNames,
                                            [&rm](const std::string& name) { return rm.GetModel(name); }))
            {
                lod.lodModels[i] = currentModel;
            }

            float distance = std::sqrt(std::max(lod.lodDistancesSq[i], 0.0f));
            if (ImGui::DragFloat("Distance", &distance, 0.25f, 0.0f, 100000.0f, "%.2f"))
            {
                distance = std::max(distance, 0.0f);
                lod.lodDistancesSq[i] = distance * distance;
            }
            ImGui::TextDisabled("DistanceSq: %.2f", lod.lodDistancesSq[i]);
            ImGui::Separator();
            ImGui::PopID();
        }

        if (removeIndex >= 0)
        {
            lod.lodModels.erase(lod.lodModels.begin() + removeIndex);
            lod.lodDistancesSq.erase(lod.lodDistancesSq.begin() + removeIndex);
        }

        if (ImGui::Button("+ Add LOD Pair"))
        {
            float previousDistance = lod.lodDistancesSq.empty() ? 0.0f : std::sqrt(lod.lodDistancesSq.back());
            float nextDistance = previousDistance + 10.0f;
            lod.lodModels.push_back(nullptr);
            lod.lodDistancesSq.push_back(nextDistance * nextDistance);
        }

        ImGui::SameLine();
        if (ImGui::Button("Sort by Distance"))
        {
            std::vector<std::pair<float, std::shared_ptr<Model>>> pairs;
            pairs.reserve(lod.lodModels.size());
            for (size_t i = 0; i < lod.lodModels.size(); ++i)
                pairs.emplace_back(lod.lodDistancesSq[i], lod.lodModels[i]);

            std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            for (size_t i = 0; i < pairs.size(); ++i)
            {
                lod.lodDistancesSq[i] = pairs[i].first;
                lod.lodModels[i] = pairs[i].second;
            }
        }
    });

    DrawComponent<LightProbeComponent>("Light Probe", reg, entity, [](auto& lp) {
        ImGui::DragFloat("Intensity##LP", &lp.intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Radius##LP", &lp.radius, 0.5f, 0.0f, 500.0f);
        ImGui::Text("SH Coefficients: 9 bands");
    });

    DrawComponent<SkyboxRenderComponent>("Skybox Render", reg, entity, [&reg, entity](auto& skyboxComp) {
        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto skyboxNames = rm.GetLoadedSkyboxes();
        auto shaderNames = rm.GetLoadedShaders();

        if (ImGui::Checkbox("Is Primary", &skyboxComp.isPrimary))
        {
            if (skyboxComp.isPrimary)
            {
                auto& scene = ServiceLocator::Instance().Require<Scene>();
                scene.SetActiveSkybox(entity);
            }
        }

        std::shared_ptr<Skybox> currentSkybox = skyboxComp.skybox;
        if (DrawResourceDropdown<Skybox>("Skybox", currentSkybox, skyboxNames,
                                         [&rm](const std::string& name) { return rm.GetSkybox(name); }))
        {
            skyboxComp.skybox = currentSkybox;
            if (skyboxComp.shader.expired() || skyboxComp.shader.lock() == nullptr)
            {
                skyboxComp.shader = rm.GetShader("axis_skybox");
            }
        }

        std::shared_ptr<Shader> currentShader = skyboxComp.shader.lock();
        if (DrawResourceDropdown<Shader>("Shader", currentShader, shaderNames,
                                         [&rm](const std::string& name) { return rm.GetShader(name); }))
        {
            skyboxComp.shader = currentShader;
        }
    });

    DrawComponent<FragmentComponent>("Fragment", reg, entity, [](auto& frag) {
        char buffer[512];
        strncpy(buffer, frag.path.c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText("Scene Path", buffer, sizeof(buffer)))
        {
            frag.path = buffer;
            frag.instantiated = false;
        }

        ImGui::Text("Status: %s", frag.instantiated ? "Instantiated" : "Pending");

        ImGui::Separator();
        ImGui::Text("Overrides:");

        auto& rm = ServiceLocator::Instance().Require<ResourceManager>();
        auto asset = rm.GetFragment(frag.path);

        if (asset)
        {
            std::vector<YAMLNode> overrideRoots;
            if (!frag.overrides.empty())
            {
                overrideRoots = YAMLParser::ParseString(frag.overrides);
            }

            const YAMLNode* entitiesNode = nullptr;
            for (auto& root : asset->rootNodes)
            {
                if (root.key == "Entities")
                {
                    entitiesNode = &root;
                    break;
                }
            }

            bool changed = false;
            if (entitiesNode)
            {
                for (auto& entNode : entitiesNode->children)
                {
                    if (ImGui::TreeNode(entNode.key.c_str()))
                    {
                        YAMLNode* targetEntityOverride = nullptr;
                        for (auto& root : overrideRoots)
                        {
                            if (root.key == entNode.key)
                            {
                                targetEntityOverride = &root;
                                break;
                            }
                        }

                        for (auto& compNode : entNode.children)
                        {
                            if (compNode.key == "Component")
                            {
                                std::string compName = compNode.value;
                                bool isOverridden = false;
                                YAMLNode* targetCompOverride = nullptr;

                                if (targetEntityOverride)
                                {
                                    for (auto& c : targetEntityOverride->children)
                                    {
                                        if (c.key == "Component" && c.value == compName)
                                        {
                                            isOverridden = true;
                                            targetCompOverride = &c;
                                            break;
                                        }
                                    }
                                }

                                bool toggled = isOverridden;
                                if (ImGui::Checkbox(compName.c_str(), &toggled))
                                {
                                    changed = true;
                                    if (toggled)
                                    {
                                        if (!targetEntityOverride)
                                        {
                                            overrideRoots.push_back(YAMLNode{entNode.key, "", {}});
                                            targetEntityOverride = &overrideRoots.back();
                                        }
                                        targetEntityOverride->children.push_back(YAMLNode{"Component", compName, {}});
                                        targetCompOverride = &targetEntityOverride->children.back();
                                    }
                                    else
                                    {
                                        if (targetEntityOverride)
                                        {
                                            for (auto it = targetEntityOverride->children.begin();
                                                 it != targetEntityOverride->children.end();)
                                            {
                                                if (it->key == "Component" && it->value == compName)
                                                {
                                                    it = targetEntityOverride->children.erase(it);
                                                }
                                                else
                                                {
                                                    ++it;
                                                }
                                            }

                                            if (targetEntityOverride->children.empty())
                                            {
                                                for (auto it = overrideRoots.begin(); it != overrideRoots.end();)
                                                {
                                                    if (it->key == entNode.key)
                                                    {
                                                        it = overrideRoots.erase(it);
                                                    }
                                                    else
                                                    {
                                                        ++it;
                                                    }
                                                }
                                                targetEntityOverride = nullptr;
                                            }
                                        }
                                        targetCompOverride = nullptr;
                                    }
                                }

                                if (toggled && targetCompOverride)
                                {
                                    ImGui::Indent();
                                    ImGui::PushID((entNode.key + compName).c_str());

                                    if (compName == "Renderer")
                                    {
                                        std::string colorStr = targetCompOverride->GetChildValue("Color", "1 1 1 1");
                                        float c[4] = {1, 1, 1, 1};
                                        std::stringstream ss(colorStr);
                                        ss >> c[0] >> c[1] >> c[2] >> c[3];
                                        if (ImGui::ColorEdit4("Color", c))
                                        {
                                            char buf[64];
                                            snprintf(buf, sizeof(buf), "%.3f %.3f %.3f %.3f", c[0], c[1], c[2], c[3]);
                                            bool found = false;
                                            for (auto& prop : targetCompOverride->children)
                                            {
                                                if (prop.key == "Color")
                                                {
                                                    prop.value = buf;
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            if (!found)
                                                targetCompOverride->children.push_back({"Color", buf, {}});
                                            changed = true;
                                        }
                                    }
                                    else if (compName == "Transform")
                                    {
                                        auto drawVec3 = [&](const char* label, const char* key, const char* def) {
                                            std::string valStr = targetCompOverride->GetChildValue(key, def);
                                            float v[3] = {0, 0, 0};
                                            if (std::string(def) == "1 1 1")
                                            {
                                                v[0] = 1;
                                                v[1] = 1;
                                                v[2] = 1;
                                            }
                                            std::stringstream ss(valStr);
                                            ss >> v[0] >> v[1] >> v[2];
                                            if (ImGui::DragFloat3(label, v, 0.1f))
                                            {
                                                char buf[64];
                                                snprintf(buf, sizeof(buf), "%.3f %.3f %.3f", v[0], v[1], v[2]);
                                                bool found = false;
                                                for (auto& prop : targetCompOverride->children)
                                                {
                                                    if (prop.key == key)
                                                    {
                                                        prop.value = buf;
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                                if (!found)
                                                    targetCompOverride->children.push_back({key, buf, {}});
                                                changed = true;
                                            }
                                        };
                                        drawVec3("Position", "Position", "0 0 0");
                                        drawVec3("Rotation", "Rotation", "0 0 0");
                                        drawVec3("Scale", "Scale", "1 1 1");
                                    }
                                    else if (compName == "Material")
                                    {
                                        auto drawFloat = [&](const char* label, const char* key, const char* def) {
                                            std::string valStr = targetCompOverride->GetChildValue(key, def);
                                            float v = 0.0f;
                                            std::stringstream ssF(valStr);
                                            ssF >> v;
                                            if (ImGui::DragFloat(label, &v, 0.01f))
                                            {
                                                char buf[64];
                                                snprintf(buf, sizeof(buf), "%.3f", v);
                                                bool found = false;
                                                for (auto& prop : targetCompOverride->children)
                                                {
                                                    if (prop.key == key)
                                                    {
                                                        prop.value = buf;
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                                if (!found)
                                                    targetCompOverride->children.push_back({key, buf, {}});
                                                changed = true;
                                            }
                                        };
                                        drawFloat("Opacity", "Opacity", "1.0");
                                        drawFloat("Roughness", "Roughness", "0.5");
                                        drawFloat("Metallic", "Metallic", "0.0");
                                        drawFloat("AO", "AO", "1.0");

                                        std::string emStr = targetCompOverride->GetChildValue("Emission", "0 0 0");
                                        float em[3] = {0, 0, 0};
                                        std::stringstream ssEm(emStr);
                                        ssEm >> em[0] >> em[1] >> em[2];
                                        if (ImGui::ColorEdit3("Emission", em))
                                        {
                                            char buf[64];
                                            snprintf(buf, sizeof(buf), "%.3f %.3f %.3f", em[0], em[1], em[2]);
                                            bool found = false;
                                            for (auto& prop : targetCompOverride->children)
                                            {
                                                if (prop.key == "Emission")
                                                {
                                                    prop.value = buf;
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            if (!found)
                                                targetCompOverride->children.push_back({"Emission", buf, {}});
                                            changed = true;
                                        }
                                        auto drawVec2 = [&](const char* label, const char* key, const char* def) {
                                            std::string valStr = targetCompOverride->GetChildValue(key, def);
                                            float v[2] = {0, 0};
                                            if (std::string(def) == "1 1")
                                            {
                                                v[0] = 1;
                                                v[1] = 1;
                                            }
                                            std::stringstream ssV(valStr);
                                            ssV >> v[0] >> v[1];
                                            if (ImGui::DragFloat2(label, v, 0.01f))
                                            {
                                                char buf[64];
                                                snprintf(buf, sizeof(buf), "%.3f %.3f", v[0], v[1]);
                                                bool found = false;
                                                for (auto& prop : targetCompOverride->children)
                                                {
                                                    if (prop.key == key)
                                                    {
                                                        prop.value = buf;
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                                if (!found)
                                                    targetCompOverride->children.push_back({key, buf, {}});
                                                changed = true;
                                            }
                                        };
                                        drawVec2("UV Scale", "UVScale", "1 1");
                                        drawVec2("UV Offset", "UVOffset", "0 0");
                                    }
                                    else
                                    {
                                        std::string compStr = "";
                                        for (const auto& prop : targetCompOverride->children)
                                        {
                                            compStr += prop.key + ": " + prop.value + "\n";
                                        }
                                        char compBuf[1024];
                                        strncpy(compBuf, compStr.c_str(), sizeof(compBuf) - 1);
                                        if (ImGui::InputTextMultiline("##props", compBuf, sizeof(compBuf),
                                                                      ImVec2(-1, 80)))
                                        {
                                            targetCompOverride->children = YAMLParser::ParseString(compBuf);
                                            changed = true;
                                        }
                                    }

                                    ImGui::PopID();
                                    ImGui::Unindent();
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }

            if (changed)
            {
                frag.overrides = "";
                for (const auto& root : overrideRoots)
                {
                    frag.overrides += SerializeYAMLNode(root, 0);
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Fragment Asset Not Found");
        }

        if (ImGui::Button("Reload & Apply Overrides"))
        {
            frag.instantiated = false;
        }
    });

    DrawComponent<NetworkComponent>("Network", reg, entity, [](auto& net) {
        int nid = (int)net.networkId;
        if (ImGui::DragInt("Network ID", &nid, 1, 0, 999999))
            net.networkId = (uint32_t)nid;

        int oid = (int)net.ownerId;
        if (ImGui::DragInt("Owner ID", &oid, 1, 0, 999999))
            net.ownerId = (uint32_t)oid;

        ImGui::Checkbox("Is Local", &net.isLocal);
    });

    // ====== PHASE 3: Add Component ======
    ImGui::Separator();
    if (ImGui::Button("+ Add Component"))
    {
        EditorSystem::PushUndoState(scene);
        ImGui::OpenPopup("AddComponentPopup");
    }
    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (!reg.try_get<PositionComponent>(entity) && ImGui::Selectable("Transform"))
            EnsureTransformComponents(reg, entity);
        if (!reg.try_get<CameraComponent>(entity) && ImGui::Selectable("Camera"))
            reg.emplace<CameraComponent>(entity);
        if (!reg.try_get<MeshRendererComponent>(entity) && ImGui::Selectable("Mesh Renderer"))
        {
            EnsureTransformComponents(reg, entity);
            reg.emplace<MeshRendererComponent>(entity);
            (void)reg.get_or_emplace<MaterialComponent>(entity);
        }
        if (!reg.try_get<MaterialComponent>(entity) && ImGui::Selectable("Material"))
            reg.emplace<MaterialComponent>(entity);
        if (!reg.try_get<DirectionalLightComponent>(entity) && ImGui::Selectable("Directional Light"))
            reg.emplace<DirectionalLightComponent>(entity);
        if (!reg.try_get<PointLightComponent>(entity) && ImGui::Selectable("Point Light"))
            reg.emplace<PointLightComponent>(entity);
        if (!reg.try_get<SpotLightComponent>(entity) && ImGui::Selectable("Spot Light"))
            reg.emplace<SpotLightComponent>(entity);
        if (!reg.try_get<RigidShapeComponent>(entity) && ImGui::Selectable("Rigid Shape"))
            reg.emplace<RigidShapeComponent>(entity);
        if (!reg.try_get<RigidBodyComponent>(entity) && ImGui::Selectable("Rigid Body"))
            reg.emplace<RigidBodyComponent>(entity);
        if (!reg.try_get<ScriptComponent>(entity) && ImGui::Selectable("Script"))
            reg.emplace<ScriptComponent>(entity);
        if (!reg.try_get<ReflectiveComponent>(entity) && ImGui::Selectable("Reflective"))
            reg.emplace<ReflectiveComponent>(entity);
        if (!reg.try_get<ReflectionProbeComponent>(entity) && ImGui::Selectable("Reflection Probe"))
            reg.emplace<ReflectionProbeComponent>(entity);
        if (!reg.try_get<SkyboxRenderComponent>(entity) && ImGui::Selectable("Skybox Render"))
            reg.emplace<SkyboxRenderComponent>(entity);
        if (!reg.try_get<AudioSourceComponent>(entity) && ImGui::Selectable("Audio Source"))
            reg.emplace<AudioSourceComponent>(entity);
        if (!reg.try_get<AnimationComponent>(entity) && ImGui::Selectable("Animator"))
            reg.emplace<AnimationComponent>(entity);
        if (!reg.try_get<ParticleEmitterComponent>(entity) && ImGui::Selectable("Particle Emitter"))
            reg.emplace<ParticleEmitterComponent>(entity);
        if (!reg.try_get<PostProcessComponent>(entity) && ImGui::Selectable("Post Process"))
            reg.emplace<PostProcessComponent>(entity);
        if (!reg.try_get<DecalComponent>(entity) && ImGui::Selectable("Decal"))
            reg.emplace<DecalComponent>(entity);
        if (!reg.try_get<LODComponent>(entity) && ImGui::Selectable("LOD"))
            reg.emplace<LODComponent>(entity);
        if (!reg.try_get<LightProbeComponent>(entity) && ImGui::Selectable("Light Probe"))
            reg.emplace<LightProbeComponent>(entity);
        if (!reg.try_get<FragmentComponent>(entity) && ImGui::Selectable("Fragment"))
            reg.emplace<FragmentComponent>(entity);
        if (!reg.try_get<NetworkComponent>(entity) && ImGui::Selectable("Network"))
            reg.emplace<NetworkComponent>(entity);
        ImGui::EndPopup();
    }
}

void SceneHierarchyPanel::CreateNewEntity(Scene& scene, const std::string& sceneName)
{
    auto& sm = ServiceLocator::Instance().Require<SceneManager>();
    std::string targetScene = sceneName.empty() ? sm.GetActiveScene() : sceneName;
    if (targetScene.empty())
        targetScene = "main";

    auto entity = scene.GetRegistry().create();
    scene.AddComponent<PositionComponent>(entity);
    scene.AddComponent<RotationComponent>(entity);
    scene.AddComponent<ScaleComponent>(entity, glm::vec3(1.0f));
    scene.AddComponent<HierarchyComponent>(entity);
    scene.AddComponent<WorldTransformComponent>(entity);
    scene.AddComponent<InfoComponent>(entity, "New Entity", "default");

    // Assign to correct scene
    scene.GetComponent<InfoComponent>(entity).sceneName = targetScene;
    sm.AddEntity(entity, targetScene);
    MarkTransformGraphDirty();
    SetSelectedEntity(entity);
}

void SceneHierarchyPanel::DuplicateEntity(Scene& scene, entt::entity srcEntity)
{
    if (srcEntity == entt::null || !scene.IsValid(srcEntity))
        return;

    auto newEntity = scene.GetRegistry().create();
    auto& reg = scene.GetRegistry();

// Helper macro to copy component if exists
#define COPY_COMP(Type)              \
    if (reg.all_of<Type>(srcEntity)) \
    reg.emplace<Type>(newEntity, reg.get<Type>(srcEntity))

    COPY_COMP(InfoComponent);
    if (reg.all_of<InfoComponent>(newEntity))
    {
        reg.get<InfoComponent>(newEntity).name += " (Copy)";
    }

    COPY_COMP(PositionComponent);
    COPY_COMP(RotationComponent);
    COPY_COMP(ScaleComponent);
    COPY_COMP(WorldTransformComponent);
    if (auto* world = reg.try_get<WorldTransformComponent>(newEntity))
        world->isDirty = true;
    COPY_COMP(MeshRendererComponent);
    COPY_COMP(MaterialComponent);
    COPY_COMP(DirectionalLightComponent);
    COPY_COMP(PointLightComponent);
    COPY_COMP(SpotLightComponent);
    COPY_COMP(CameraComponent);
    if (reg.all_of<ScriptComponent>(srcEntity))
    {
        auto& srcSc = reg.get<ScriptComponent>(srcEntity);
        auto& newSc = reg.emplace<ScriptComponent>(newEntity);
        newSc.className = srcSc.className;
        newSc.InstantiateScript = srcSc.InstantiateScript;
        newSc.DestroyScript = srcSc.DestroyScript;
    }
    COPY_COMP(UITextComponent);
    COPY_COMP(AudioSourceComponent);
    COPY_COMP(RigidShapeComponent);
    COPY_COMP(RigidBodyComponent);
    COPY_COMP(SkyboxRenderComponent);
    COPY_COMP(FragmentComponent);
    COPY_COMP(NetworkComponent);
    COPY_COMP(AnimationComponent);
    if (reg.all_of<ParticleEmitterComponent>(srcEntity))
    {
        auto& srcPe = reg.get<ParticleEmitterComponent>(srcEntity);
        auto& dstPe = reg.emplace<ParticleEmitterComponent>(newEntity);
        dstPe.isActive = srcPe.isActive;
        dstPe.emissionRate = srcPe.emissionRate;
        dstPe.lifetime = srcPe.lifetime;
        dstPe.speed = srcPe.speed;
        dstPe.size = srcPe.size;
        dstPe.direction = srcPe.direction;
        dstPe.spread = srcPe.spread;
        dstPe.startColor = srcPe.startColor;
        dstPe.endColor = srcPe.endColor;
        dstPe.textureName = srcPe.textureName;
        dstPe.customShader = srcPe.customShader;
    }
    COPY_COMP(PostProcessComponent);
    COPY_COMP(DecalComponent);
    COPY_COMP(TerrainComponent);
    COPY_COMP(LightProbeComponent);
    COPY_COMP(ReflectionProbeComponent);
    // Setup hierarchy properly (don't copy children directly, just attach to same parent)
    reg.emplace<HierarchyComponent>(newEntity);
    if (reg.all_of<HierarchyComponent>(srcEntity))
    {
        auto parent = reg.get<HierarchyComponent>(srcEntity).parent;
        if (parent != entt::null)
        {
            scene.AddChild(parent, newEntity, false);
        }
    }

    auto* sceneMgr = ServiceLocator::Instance().Resolve<SceneManager>();
    if (sceneMgr)
    {
        std::string sceneName = sceneMgr->GetActiveScene();
        if (auto* info = reg.try_get<InfoComponent>(newEntity))
        {
            if (!info->sceneName.empty())
                sceneName = info->sceneName;
        }
        if (sceneName.empty())
            sceneName = "main";
        sceneMgr->AddEntity(newEntity, sceneName);
    }

    SetSelectedEntity(newEntity);
    MarkTransformGraphDirty();
}

#endif
