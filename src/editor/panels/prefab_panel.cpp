#include <editor/panels/prefab_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <core/logic/filesystem.h>
#include <audio/logic/audio_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/fragment_component.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <core/logic/yaml_parser.h>
#include <imgui.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace
{
SceneSerializer* CreateSerializer(ResourceManager& resources, std::unique_ptr<SceneSerializer>& storage)
{
    auto& services = ServiceLocator::Instance();
    storage = std::make_unique<SceneSerializer>(
        resources, services.Resolve<IPhysicsWorld>(), services.Resolve<AudioService>());
    return storage.get();
}

std::vector<entt::entity> ChildRoots(Scene& scene, entt::entity entity)
{
    if (const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(entity))
        return hierarchy->children;
    return {};
}

std::vector<std::string> DiscoverPrefabAssets()
{
    std::vector<std::string> result;
    const std::filesystem::path root = FileSystem::getPath("assets/prefabs");
    std::error_code error;
    if (!std::filesystem::exists(root, error))
        return result;
    for (std::filesystem::recursive_directory_iterator iterator(
             root, std::filesystem::directory_options::skip_permission_denied, error), end;
         iterator != end; iterator.increment(error))
    {
        if (error)
        {
            error.clear();
            continue;
        }
        if (!iterator->is_regular_file(error) || iterator->path().extension() != ".axs")
            continue;
        const auto roots = YAMLParser::Parse(iterator->path().generic_string());
        if (std::any_of(roots.begin(), roots.end(), [](const YAMLNode& node) { return node.key == "Entities"; }))
            result.push_back(FileSystem::getRelativePath(iterator->path().generic_string()));
    }
    std::sort(result.begin(), result.end());
    return result;
}
}

void PrefabPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
    const entt::entity selected = selection ? selection->GetPrimary() : entt::null;
    auto* fragment = scene.IsValid(selected) ? scene.TryGetComponent<FragmentComponent>(selected) : nullptr;

    ImGui::SeparatorText("Create prefab");
    ImGui::TextWrapped("Create an .axs asset from the selected entity and its children.");
    const bool hasSource = scene.IsValid(selected);
    ImGui::BeginDisabled(!hasSource);
    if (ImGui::Button("Create prefab from selection..."))
    {
        m_CreateSource = selected;
        m_AllowOverwrite = false;
        ImGui::OpenPopup("Create prefab asset");
    }
    ImGui::EndDisabled();
    if (!hasSource)
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Select a source entity in Hierarchy first.");
    if (ImGui::Button("Create instance from existing..."))
    {
        m_DiscoveredPrefabs = DiscoverPrefabAssets();
        ImGui::OpenPopup("Open prefab asset");
    }
    if (ImGui::BeginPopupModal("Create prefab asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const bool sourceValid = scene.IsValid(m_CreateSource);
        std::string sourceName = "Selection no longer exists";
        if (sourceValid)
        {
            sourceName = "Entity " + std::to_string(entt::to_integral(m_CreateSource));
            if (const auto* info = scene.TryGetComponent<InfoComponent>(m_CreateSource))
                sourceName = info->name;
        }
        ImGui::Text("Source: %s", sourceName.c_str());
        ImGui::InputText("Save path (.axs)", m_Path.data(), m_Path.size());
        ImGui::Checkbox("Replace selection with linked instance", &m_ReplaceSelection);
        const std::filesystem::path proposed = FileSystem::getPath(m_Path.data());
        const bool validPath = !proposed.empty() && proposed.extension() == ".axs";
        const bool exists = std::filesystem::exists(proposed);
        if (!validPath)
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "A prefab path must end in .axs.");
        if (exists)
            ImGui::Checkbox("Overwrite existing prefab", &m_AllowOverwrite);
        ImGui::BeginDisabled(!validPath || !sourceValid || (exists && !m_AllowOverwrite));
        if (ImGui::Button("Create"))
        {
            const bool success = WriteSelection(scene, m_CreateSource, m_ReplaceSelection);
            m_Status = success ? (m_ReplaceSelection ? "Prefab created and linked instance selected."
                                                     : "Prefab asset created.")
                               : "Could not create prefab.";
            if (success)
            {
                fragment = nullptr;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Open prefab asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextDisabled("Prefab assets under assets/prefabs");
        ImGui::BeginChild("PrefabAssetFiles", ImVec2(520.0f, 260.0f), true);
        if (m_DiscoveredPrefabs.empty())
            ImGui::TextDisabled("No .axs prefab assets found.");
        for (const std::string& file : m_DiscoveredPrefabs)
        {
            if (ImGui::Selectable(file.c_str()))
            {
                m_Status = CreateInstance(scene, file) ? "Prefab instance created from " + file + "."
                                                       : "Could not create prefab instance.";
                if (m_Status.rfind("Prefab instance", 0) == 0)
                    ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Refresh"))
            m_DiscoveredPrefabs = DiscoverPrefabAssets();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SeparatorText("Selected instance");
    if (!fragment)
    {
        ImGui::TextDisabled("Select an entity with a Fragment/Prefab component.");
    }
    else
    {
        ImGui::Text("Asset: %s", fragment->path.empty() ? "(not assigned)" : fragment->path.c_str());
        const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(selected);
        ImGui::Text("Children: %zu", hierarchy ? hierarchy->children.size() : 0);
        ImGui::SameLine();
        ImGui::TextColored(fragment->instantiated ? ImVec4(0.35f, 0.9f, 0.45f, 1.0f)
                                                  : ImVec4(1.0f, 0.7f, 0.25f, 1.0f),
                           "%s", fragment->instantiated ? "Loaded" : "Reload pending");
        ImGui::Text("Overrides: %s", fragment->overrides.empty() ? "none" : "present");

        if (ImGui::Button("Apply instance to asset"))
            m_Status = ApplyInstance(scene) ? "Prefab changes written to the .axs asset."
                                             : "Prefab has no writable asset or instantiated children.";
        ImGui::SameLine();
        if (ImGui::Button("Revert from asset"))
            m_Status = RevertInstance(scene) ? "Overrides cleared; prefab queued for reload."
                                              : "Could not revert this prefab.";

        if (selected != m_OverrideEntity)
        {
            m_OverrideEntity = selected;
            std::strncpy(m_OverrideText.data(), fragment->overrides.c_str(), m_OverrideText.size() - 1);
            m_OverrideText.back() = '\0';
        }
        if (ImGui::CollapsingHeader("Advanced override YAML"))
        {
            ImGui::TextWrapped("Overrides are applied after the base prefab loads. Use the Inspector's visual "
                               "override controls for common components.");
            ImGui::InputTextMultiline("##override_yaml", m_OverrideText.data(), m_OverrideText.size(),
                                      ImVec2(-1.0f, 160.0f));
            if (ImGui::Button("Validate and apply overrides"))
            {
                const std::string candidate = m_OverrideText.data();
                const auto roots = YAMLParser::ParseString(candidate);
                if (!candidate.empty() && roots.empty())
                    m_Status = "Override YAML is invalid or empty after parsing.";
                else
                {
                    EditorSystem::BeginTransaction(scene, "Edit prefab overrides");
                    fragment->overrides = candidate;
                    fragment->instantiated = false;
                    m_Status = "Overrides accepted; prefab queued for reload.";
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear overrides"))
            {
                EditorSystem::BeginTransaction(scene, "Clear prefab overrides");
                fragment->overrides.clear();
                fragment->instantiated = false;
                m_OverrideText.fill('\0');
                m_Status = "Overrides cleared.";
            }
        }
    }
    if (!m_Status.empty())
        ImGui::TextWrapped("%s", m_Status.c_str());
    ImGui::End();
}

bool PrefabPanel::WriteSelection(Scene& scene, entt::entity source, bool replaceSelection)
{
    auto& services = ServiceLocator::Instance();
    auto* selection = services.Resolve<EditorSelection>();
    auto* resources = services.Resolve<ResourceManager>();
    if (!selection || !resources || !scene.IsValid(source))
        return false;

    const std::string resourceName = std::filesystem::path(m_Path.data()).lexically_normal().generic_string();
    const std::filesystem::path path = std::filesystem::path(FileSystem::getPath(resourceName)).lexically_normal();
    if (path.empty() || path.extension() != ".axs")
        return false;

    std::unique_ptr<SceneSerializer> serializerStorage;
    auto* serializer = CreateSerializer(*resources, serializerStorage);
    const entt::entity selected = source;
    const std::string content = serializer->SerializeEntitiesToString(scene, {selected});
    if (content.empty())
        return false;

    std::error_code error;
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path, std::ios::trunc);
    if (error || !file || !(file << content))
        return false;
    file.close();

    resources->UnloadFragment(resourceName);
    resources->LoadFragment(resourceName, resourceName);
    if (!replaceSelection)
        return true;

    EditorSystem::BeginTransaction(scene, "Create prefab instance");
    std::string name = "Prefab Instance";
    std::string sceneName;
    entt::entity parent = entt::null;
    if (const auto* info = scene.TryGetComponent<InfoComponent>(selected))
    {
        name = info->name + " Instance";
        sceneName = info->sceneName;
    }
    if (const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(selected))
        parent = hierarchy->parent;
    const entt::entity anchor = scene.CreateEmptyEntity(name);
    if (auto* info = scene.TryGetComponent<InfoComponent>(anchor))
        info->sceneName = sceneName;
    scene.AddComponent<FragmentComponent>(anchor, resourceName, std::string{}, false);
    if (scene.IsValid(parent))
        scene.SetParent(anchor, parent, false);
    scene.DestroyEntityWithChildren(selected, services.Resolve<SceneManager>());
    if (auto* manager = services.Resolve<SceneManager>(); manager && !sceneName.empty())
        manager->AddEntity(anchor, sceneName);
    selection->Select(scene, anchor);
    return true;
}

bool PrefabPanel::CreateInstance(Scene& scene, const std::string& path)
{
    auto& services = ServiceLocator::Instance();
    auto* selection = services.Resolve<EditorSelection>();
    auto* resources = services.Resolve<ResourceManager>();
    if (!selection || !resources || path.empty() || !std::filesystem::exists(FileSystem::getPath(path)))
        return false;
    EditorSystem::BeginTransaction(scene, "Create prefab instance");
    const entt::entity anchor = scene.CreateEmptyEntity(
        std::filesystem::path(path).stem().string() + " Instance");
    scene.AddComponent<FragmentComponent>(anchor, path, std::string{}, false);
    resources->UnloadFragment(path);
    resources->LoadFragment(path, path);
    selection->Select(scene, anchor);
    return true;
}

bool PrefabPanel::ApplyInstance(Scene& scene)
{
    auto& services = ServiceLocator::Instance();
    auto* selection = services.Resolve<EditorSelection>();
    auto* resources = services.Resolve<ResourceManager>();
    if (!selection || !resources || !scene.IsValid(selection->GetPrimary()))
        return false;
    const entt::entity anchor = selection->GetPrimary();
    auto* fragment = scene.TryGetComponent<FragmentComponent>(anchor);
    const auto roots = ChildRoots(scene, anchor);
    if (!fragment || fragment->path.empty() || roots.empty())
        return false;

    std::string prefix;
    if (const auto* info = scene.TryGetComponent<InfoComponent>(anchor))
        prefix = info->name + ".";
    std::unique_ptr<SceneSerializer> serializerStorage;
    const std::string content =
        CreateSerializer(*resources, serializerStorage)->SerializeEntitiesToString(scene, roots, prefix);
    const std::filesystem::path outputPath = FileSystem::getPath(fragment->path);
    std::error_code error;
    if (!outputPath.parent_path().empty())
        std::filesystem::create_directories(outputPath.parent_path(), error);
    std::ofstream file(outputPath, std::ios::trunc);
    if (!file || !(file << content))
        return false;
    file.close();

    EditorSystem::BeginTransaction(scene, "Apply prefab");
    fragment->overrides.clear();
    fragment->instantiated = false;
    resources->UnloadFragment(fragment->path);
    resources->LoadFragment(fragment->path, fragment->path);
    return true;
}

bool PrefabPanel::RevertInstance(Scene& scene)
{
    auto& services = ServiceLocator::Instance();
    auto* selection = services.Resolve<EditorSelection>();
    if (!selection || !scene.IsValid(selection->GetPrimary()))
        return false;
    auto* fragment = scene.TryGetComponent<FragmentComponent>(selection->GetPrimary());
    if (!fragment || fragment->path.empty())
        return false;
    EditorSystem::BeginTransaction(scene, "Revert prefab");
    fragment->overrides.clear();
    fragment->instantiated = false;
    if (auto* resources = services.Resolve<ResourceManager>())
        resources->ReimportResource(fragment->path);
    return true;
}

#endif
