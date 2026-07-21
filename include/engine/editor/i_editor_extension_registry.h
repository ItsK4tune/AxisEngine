#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class IEditorModule;
class IEditorPanel;

enum class EditorExtensionKind
{
    Module,
    Panel
};

struct EditorExtensionInfo
{
    std::string owner;
    std::string name;
    EditorExtensionKind kind = EditorExtensionKind::Module;
};

class IEditorExtensionRegistry
{
public:
    using ModuleFactory = std::function<std::unique_ptr<IEditorModule>()>;
    using PanelFactory = std::function<std::unique_ptr<IEditorPanel>()>;

    virtual ~IEditorExtensionRegistry() = default;
    virtual bool RegisterModule(std::string owner, std::string name, ModuleFactory factory) = 0;
    virtual bool RegisterPanel(std::string owner, std::string name, PanelFactory factory) = 0;
    virtual size_t UnregisterOwner(std::string_view owner) = 0;
    virtual std::vector<EditorExtensionInfo> GetExtensions() const = 0;
};
