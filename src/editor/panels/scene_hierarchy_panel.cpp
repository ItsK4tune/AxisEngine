#include <editor/panels/scene_hierarchy_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <scene/logic/scene.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/ui_components.h>
#include <script/logic/scriptable.h>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>

void SceneHierarchyPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto& registry = scene.registry;

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

    ImGui::Columns(2, "HierarchyInspector", true);
    ImGui::BeginChild("HierarchyTree");

    for (const auto& [sname, entities] : sceneGroups)
    {
        if (ImGui::TreeNodeEx(sname.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            for (auto entity : entities)
            {
                DrawEntityNode(scene, entity);
            }
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("InspectorView");
    if (m_SelectedEntity != entt::null && registry.valid(m_SelectedEntity))
    {
        DrawComponents(registry, m_SelectedEntity);
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
    auto& registry = scene.registry;
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
        ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entityId, flags, "%s", name.c_str());
    if (ImGui::IsItemClicked())
    {
        m_SelectedEntity = entity;
    }

    if (opened && hasChildren && children)
    {
        for (auto child : *children)
        {
            DrawEntityNode(scene, child);
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::DrawComponents(entt::registry& reg, entt::entity entity)
{
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

    if (auto* cam = reg.try_get<CameraComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Camera"))
        {
            ImGui::Checkbox("Primary", &cam->isPrimary);
            ImGui::DragFloat("FOV", &cam->fov, 0.1f);
            ImGui::DragFloat("Near Clip", &cam->nearPlane, 0.1f);
            ImGui::DragFloat("Far Clip", &cam->farPlane, 0.1f);
        }
    }

    if (auto* mesh = reg.try_get<MeshRendererComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Mesh Renderer"))
        {
            ImGui::ColorEdit4("Color", &mesh->color.r);
            ImGui::Checkbox("Cast Shadow", &mesh->castShadow);
        }
    }

    if (auto* light = reg.try_get<DirectionalLightComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Directional Light"))
        {
            ImGui::DragFloat3("Direction", &light->direction.x, 0.1f);
            ImGui::ColorEdit3("Color", &light->color.r);
            ImGui::DragFloat("Intensity", &light->intensity, 0.1f);
            ImGui::Checkbox("Cast Shadow", &light->isCastShadow);
        }
    }

    if (auto* light = reg.try_get<PointLightComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Point Light"))
        {
            ImGui::ColorEdit3("Color", &light->color.r);
            ImGui::DragFloat("Intensity", &light->intensity, 0.1f);
            ImGui::DragFloat("Radius", &light->radius, 0.5f);
            ImGui::Checkbox("Cast Shadow", &light->isCastShadow);
        }
    }

    if (auto* light = reg.try_get<SpotLightComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Spot Light"))
        {
            ImGui::DragFloat3("Direction", &light->direction.x, 0.1f);
            ImGui::ColorEdit3("Color", &light->color.r);
            ImGui::DragFloat("Intensity", &light->intensity, 0.1f);
            ImGui::Checkbox("Cast Shadow", &light->isCastShadow);
        }
    }

    if (auto* rb = reg.try_get<RigidBodyComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Rigid Body"))
        {
            ImGui::DragFloat("Mass", &rb->mass, 0.1f, 0.0f, 10000.0f);
            ImGui::Checkbox("Kinematic", &rb->isKinematic);
            ImGui::Checkbox("Static", &rb->isStatic);
            ImGui::Checkbox("Trigger", &rb->isTrigger);
            ImGui::DragFloat3("Linear Factor", &rb->linearFactor.x, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat3("Angular Factor", &rb->angularFactor.x, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Linear Damping", &rb->linearDamping, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Angular Damping", &rb->angularDamping, 0.01f, 0.0f, 1.0f);
        }
    }

    if (auto* cc = reg.try_get<CharacterControllerComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Character Controller"))
        {
            ImGui::DragFloat("Step Height", &cc->stepHeight, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Max Slope", &cc->maxSlope, 0.5f, 0.0f, 90.0f);
            ImGui::Text("On Ground: %s", cc->isOnGround ? "Yes" : "No");
        }
    }

    if (auto* mat = reg.try_get<AxisMaterialComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Material"))
        {
            static const char* matTypes[] = {"Phong", "PBR"};
            int currentType = (int)mat->desc.type;
            if (ImGui::Combo("Type", &currentType, matTypes, IM_ARRAYSIZE(matTypes)))
            {
                mat->desc.type = (AxisMaterialType)currentType;
            }
            ImGui::DragFloat("Opacity", &mat->desc.opacity, 0.01f, 0.0f, 1.0f);
            ImGui::ColorEdit3("Emission", &mat->desc.emission.r);
            ImGui::DragFloat2("UV Scale", &mat->desc.uvScale.x, 0.01f);
            ImGui::DragFloat2("UV Offset", &mat->desc.uvOffset.x, 0.01f);
            if (mat->desc.type == AxisMaterialType::PBR)
            {
                ImGui::Separator();
                ImGui::DragFloat("Roughness", &mat->desc.pbr.roughness, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Metallic", &mat->desc.pbr.metallic, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("AO", &mat->desc.pbr.ao, 0.01f, 0.0f, 1.0f);
            }
            else
            {
                ImGui::Separator();
                ImGui::DragFloat("Shininess", &mat->desc.phong.shininess, 1.0f, 1.0f, 512.0f);
            }
        }
    }

    if (auto* sc = reg.try_get<ScriptComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Script"))
        {
            if (sc->instance)
            {
                ImGui::Text("Script: Active");
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Script: Not Instantiated");
            }
        }
    }

    if (auto* uiTransform = reg.try_get<UITransformComponent>(entity))
    {
        if (ImGui::CollapsingHeader("UI Transform"))
        {
            auto DrawVec2Percent = [](const char* label, glm::vec2& v, glm::bvec2& p, float speed) {
                ImGui::PushID(label);
                ImGui::DragFloat2(label, &v.x, speed);
                ImGui::SameLine();
                ImGui::Checkbox("X%", &p.x);
                ImGui::SameLine();
                ImGui::Checkbox("Y%", &p.y);
                ImGui::PopID();
            };

            DrawVec2Percent("Position##UI", uiTransform->position, uiTransform->positionIsPercent, 1.0f);
            DrawVec2Percent("Size##UI", uiTransform->size, uiTransform->sizeIsPercent, 1.0f);

            ImGui::DragFloat("Rotation##UI", &uiTransform->rotation, 1.0f);
            ImGui::DragInt("Z-Index##UI", &uiTransform->zIndex);
            ImGui::DragFloat2("Pivot##UI", &uiTransform->pivot.x, 0.05f);

            DrawVec2Percent("Anchor Min##UI", uiTransform->anchorMin, uiTransform->anchorMinIsPercent, 0.05f);
            DrawVec2Percent("Anchor Max##UI", uiTransform->anchorMax, uiTransform->anchorMaxIsPercent, 0.05f);
            DrawVec2Percent("Offset Min##UI", uiTransform->offsetMin, uiTransform->offsetMinIsPercent, 1.0f);
            DrawVec2Percent("Offset Max##UI", uiTransform->offsetMax, uiTransform->offsetMaxIsPercent, 1.0f);
        }
    }

    if (auto* uiFlex = reg.try_get<UIFlexLayoutComponent>(entity))
    {
        if (ImGui::CollapsingHeader("UI Flex Layout"))
        {
            static const char* flexDirs[] = {"Row", "Column"};
            int currentDir = (int)uiFlex->direction;
            if (ImGui::Combo("Direction##UI", &currentDir, flexDirs, IM_ARRAYSIZE(flexDirs)))
            {
                uiFlex->direction = (FlexDirection)currentDir;
            }
            ImGui::DragFloat("Spacing##UI", &uiFlex->spacing, 1.0f);
            ImGui::Checkbox("Auto Size##UI", &uiFlex->autoSize);
            ImGui::DragFloat4("Padding##UI", &uiFlex->padding.x, 1.0f);
        }
    }

    if (auto* uiRenderer = reg.try_get<UIRendererComponent>(entity))
    {
        if (ImGui::CollapsingHeader("UI Renderer"))
        {
            ImGui::ColorEdit4("Color##UI", &uiRenderer->color.r);
            ImGui::Text("Model: %s", uiRenderer->model ? "Loaded" : "None");
            ImGui::Text("Shader: %s", uiRenderer->shader ? "Loaded" : "None");
            ImGui::Text("Texture: %s", uiRenderer->texture ? "Loaded" : "None");
        }
    }

    if (auto* uiText = reg.try_get<UITextComponent>(entity))
    {
        if (ImGui::CollapsingHeader("UI Text"))
        {
            char buffer[256];
            strncpy(buffer, uiText->text.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText("Text##UI", buffer, sizeof(buffer)))
            {
                uiText->text = buffer;
            }
            ImGui::ColorEdit4("Color##UIText", &uiText->color.r);
            ImGui::DragFloat("Scale##UIText", &uiText->scale, 0.01f);

            static const char* alignments[] = {"Left", "Center", "Right"};
            int currentAlign = (int)uiText->alignment;
            if (ImGui::Combo("Alignment##UI", &currentAlign, alignments, IM_ARRAYSIZE(alignments)))
            {
                uiText->alignment = (TextAlignment)currentAlign;
            }

            ImGui::Checkbox("Word Wrap##UI", &uiText->wordWrap);
            if (uiText->wordWrap)
            {
                ImGui::DragFloat("Max Width##UI", &uiText->maxWidth, 1.0f);
            }
            ImGui::Text("Font: %s", uiText->fontName.empty() ? "None" : uiText->fontName.c_str());
        }
    }
}
#endif
