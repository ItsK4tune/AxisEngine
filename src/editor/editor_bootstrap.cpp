#include <ecs/logic/system_factory.h>
#include <editor/editor_system.h>

#include <memory>

// Axis::Editor's interface link options retain this object from the static
// archive. Its initializer runs before Application::Initialize(), so the
// normal deterministic SystemFactory pass creates EditorSystem whenever the
// editor library is linked. axis_engine itself remains editor-independent.
namespace
{
[[maybe_unused]] const bool EditorSystemRegistered =
    SystemFactory::Register("EditorSystem", [] { return std::make_unique<EditorSystem>(); });
}

// Stable C-linkage anchor used by CMake to retain only this translation unit;
// forcing the whole editor archive would unnecessarily increase link time and
// executable size.
extern "C" void axis_editor_bootstrap_anchor()
{
}
