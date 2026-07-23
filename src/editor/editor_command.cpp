#include <editor/editor_command.h>

#ifdef ENABLE_EDITOR

#include <utility>

namespace
{
class SceneStateCommand final : public IEditorCommand
{
public:
    SceneStateCommand(std::string name, std::string before, std::string after,
                      EditorCommandHistory::RestoreScene restore)
        : m_Name(std::move(name)),
          m_Before(std::move(before)),
          m_After(std::move(after)),
          m_Restore(std::move(restore))
    {
    }

    const char* GetName() const override
    {
        return m_Name.c_str();
    }

    bool Redo(Scene& scene) override
    {
        return m_Restore && m_Restore(scene, m_After);
    }

    bool Undo(Scene& scene) override
    {
        return m_Restore && m_Restore(scene, m_Before);
    }

private:
    std::string m_Name;
    std::string m_Before;
    std::string m_After;
    EditorCommandHistory::RestoreScene m_Restore;
};
}  // namespace

bool EditorCommandHistory::Execute(Scene& scene, std::unique_ptr<IEditorCommand> command)
{
    if (!command || !command->Redo(scene))
        return false;
    PushUndo(std::move(command));
    m_Redo.clear();
    return true;
}

bool EditorCommandHistory::CommitExecuted(Scene& scene, std::unique_ptr<IEditorCommand> command,
                                          const CaptureScene& capture)
{
    if (!command)
        return false;
    FinalizePending(scene, capture);
    PushUndo(std::move(command));
    m_Redo.clear();
    return true;
}

void EditorCommandHistory::BeginSceneTransaction(Scene& scene, std::string name,
                                                 const CaptureScene& capture, RestoreScene restore)
{
    FinalizePending(scene, capture);
    m_PendingBefore = capture ? capture(scene) : std::string{};
    m_PendingName = std::move(name);
    m_PendingRestore = std::move(restore);
    m_Redo.clear();
}

void EditorCommandHistory::FinalizeSceneTransaction(Scene& scene, const CaptureScene& capture)
{
    FinalizePending(scene, capture);
}

bool EditorCommandHistory::Undo(Scene& scene, const CaptureScene& capture)
{
    FinalizePending(scene, capture);
    if (m_Undo.empty())
        return false;

    auto command = std::move(m_Undo.back());
    m_Undo.pop_back();
    if (!command->Undo(scene))
    {
        m_Undo.push_back(std::move(command));
        return false;
    }
    m_Redo.push_back(std::move(command));
    return true;
}

bool EditorCommandHistory::Redo(Scene& scene)
{
    if (m_Redo.empty())
        return false;
    auto command = std::move(m_Redo.back());
    m_Redo.pop_back();
    if (!command->Redo(scene))
    {
        m_Redo.push_back(std::move(command));
        return false;
    }
    PushUndo(std::move(command));
    return true;
}

void EditorCommandHistory::Clear()
{
    m_Undo.clear();
    m_Redo.clear();
    m_PendingName.clear();
    m_PendingBefore.clear();
    m_PendingRestore = {};
}

bool EditorCommandHistory::CanUndo() const
{
    return !m_Undo.empty() || !m_PendingBefore.empty();
}

bool EditorCommandHistory::CanRedo() const
{
    return !m_Redo.empty();
}

size_t EditorCommandHistory::GetUndoCount() const
{
    return m_Undo.size() + (m_PendingBefore.empty() ? 0u : 1u);
}

size_t EditorCommandHistory::GetRedoCount() const
{
    return m_Redo.size();
}

void EditorCommandHistory::FinalizePending(Scene& scene, const CaptureScene& capture)
{
    if (m_PendingBefore.empty())
        return;
    const std::string after = capture ? capture(scene) : std::string{};
    if (!after.empty() && after != m_PendingBefore && m_PendingRestore)
    {
        PushUndo(std::make_unique<SceneStateCommand>(
            m_PendingName.empty() ? "Scene edit" : m_PendingName, std::move(m_PendingBefore), after,
            std::move(m_PendingRestore)));
    }
    m_PendingName.clear();
    m_PendingBefore.clear();
    m_PendingRestore = {};
}

void EditorCommandHistory::PushUndo(std::unique_ptr<IEditorCommand> command)
{
    m_Undo.push_back(std::move(command));
    if (m_Undo.size() > kMaxCommands)
        m_Undo.erase(m_Undo.begin());
}

#endif
