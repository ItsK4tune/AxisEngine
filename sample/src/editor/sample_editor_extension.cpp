#include "sample_editor_extension.h"

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <ecs/unit/core_components.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <editor/i_editor_extension_registry.h>
#include <editor/i_editor_module.h>
#include <editor/i_editor_panel.h>
#include <scene/logic/scene.h>

#include <imgui.h>

#include <cstdint>
#include <memory>
#include <string>

namespace
{
constexpr char kExtensionOwner[] = "axis.samples.editor_demo";

struct DemoExtensionState
{
    std::uint64_t updateCount = 0;
    float activeSeconds = 0.0f;
    bool moduleInitialized = false;
};

class SampleEditorModule final : public IEditorModule
{
public:
    explicit SampleEditorModule(std::shared_ptr<DemoExtensionState> state) : m_State(std::move(state))
    {
    }

    void Initialize() override
    {
        m_State->moduleInitialized = true;
    }

    void Shutdown() override
    {
        m_State->moduleInitialized = false;
    }

    void OnUpdate(float dt) override
    {
        ++m_State->updateCount;
        m_State->activeSeconds += dt;
    }

private:
    std::shared_ptr<DemoExtensionState> m_State;
};

class SampleEditorPanel final : public IEditorPanel
{
public:
    explicit SampleEditorPanel(std::shared_ptr<DemoExtensionState> state) : m_State(std::move(state))
    {
    }

    void OnImGui(Scene& scene) override
    {
        ImGui::Begin(GetTitle().c_str(), &m_Open);
        ImGui::TextUnformatted("Owner: axis.samples.editor_demo");
        ImGui::Text("Module: %s", m_State->moduleInitialized ? "running" : "stopped");
        ImGui::Text("Updates: %llu", static_cast<unsigned long long>(m_State->updateCount));
        ImGui::Text("Active time: %.2f s", m_State->activeSeconds);
        ImGui::Separator();

        auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
        const std::size_t selectedCount = selection ? selection->GetAll().size() : 0;
        ImGui::Text("Selected entities: %zu", selectedCount);

        if (ImGui::Button("Create demo entity"))
        {
            EditorSystem::BeginTransaction(scene, "Demo extension: create entity");
            const entt::entity entity =
                scene.CreateEntityWithTransform("Editor Extension Demo", {0.0f, 1.0f, 0.0f});
            if (auto* info = scene.TryGetComponent<InfoComponent>(entity))
                info->tag = "editor-demo";
            if (selection)
                selection->Select(scene, entity);
        }

        const bool canNudge = selection && !selection->Empty();
        if (!canNudge)
            ImGui::BeginDisabled();
        if (ImGui::Button("Move selection +1 on X"))
        {
            EditorSystem::BeginTransaction(scene, "Demo extension: move selection");
            for (const entt::entity entity : selection->GetAll())
            {
                if (auto* position = scene.TryGetComponent<PositionComponent>(entity))
                {
                    position->value.x += 1.0f;
                    scene.MarkTransformDirty(entity);
                }
            }
        }
        if (!canNudge)
            ImGui::EndDisabled();

        ImGui::TextWrapped(
            "Both actions participate in the shared editor undo/redo history. "
            "The move action intentionally applies to the complete multi-selection.");
        ImGui::End();
    }

    std::string GetTitle() const override
    {
        return "Extension Demo";
    }

    PanelGroup GetGroup() const override
    {
        return PanelGroup::Tools;
    }

private:
    std::shared_ptr<DemoExtensionState> m_State;
};
}  // namespace

bool RegisterSampleEditorExtension()
{
    auto* registry = ServiceLocator::Instance().Resolve<IEditorExtensionRegistry>();
    if (!registry)
        return false;

    for (const auto& extension : registry->GetExtensions())
    {
        if (extension.owner == kExtensionOwner)
            return true;
    }

    auto state = std::make_shared<DemoExtensionState>();
    const bool moduleRegistered = registry->RegisterModule(
        kExtensionOwner, "sample.lifecycle",
        [state] { return std::make_unique<SampleEditorModule>(state); });
    const bool panelRegistered = registry->RegisterPanel(
        kExtensionOwner, "sample.tools",
        [state] { return std::make_unique<SampleEditorPanel>(state); });

    if (!moduleRegistered || !panelRegistered)
    {
        registry->UnregisterOwner(kExtensionOwner);
        return false;
    }
    return true;
}

void UnregisterSampleEditorExtension()
{
    if (auto* registry = ServiceLocator::Instance().Resolve<IEditorExtensionRegistry>())
        registry->UnregisterOwner(kExtensionOwner);
}

#endif
