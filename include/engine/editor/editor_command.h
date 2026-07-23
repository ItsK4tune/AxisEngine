#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Scene;

class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;
    virtual const char* GetName() const = 0;
    virtual bool Redo(Scene& scene) = 0;
    virtual bool Undo(Scene& scene) = 0;
};

class EditorCommandHistory
{
public:
    using CaptureScene = std::function<std::string(Scene&)>;
    using RestoreScene = std::function<bool(Scene&, const std::string&)>;

    bool Execute(Scene& scene, std::unique_ptr<IEditorCommand> command);
    bool CommitExecuted(Scene& scene, std::unique_ptr<IEditorCommand> command, const CaptureScene& capture);
    void BeginSceneTransaction(Scene& scene, std::string name, const CaptureScene& capture,
                               RestoreScene restore);
    void FinalizeSceneTransaction(Scene& scene, const CaptureScene& capture);
    bool Undo(Scene& scene, const CaptureScene& capture);
    bool Redo(Scene& scene);
    void Clear();

    bool CanUndo() const;
    bool CanRedo() const;
    size_t GetUndoCount() const;
    size_t GetRedoCount() const;

private:
    void FinalizePending(Scene& scene, const CaptureScene& capture);
    void PushUndo(std::unique_ptr<IEditorCommand> command);

    std::vector<std::unique_ptr<IEditorCommand>> m_Undo;
    std::vector<std::unique_ptr<IEditorCommand>> m_Redo;
    std::string m_PendingName;
    std::string m_PendingBefore;
    RestoreScene m_PendingRestore;
    static constexpr size_t kMaxCommands = 100;
};
