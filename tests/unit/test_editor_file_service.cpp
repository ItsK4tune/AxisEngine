#include "test_framework.h"
#include "test_support.h"

#ifdef ENABLE_EDITOR
#include <editor/logic/editor_file_service.h>
#include <fstream>

namespace
{
std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
}

AXIS_TEST_CASE("EditorFileService never overwrites an existing asset")
{
    const auto root = axis_test_support::TempPath("editor_file_service_no_overwrite");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    const auto asset = root / "scene.axs";

    const auto created = EditorFileService::CreateAssetFile(root, asset, "first");
    const auto rejected = EditorFileService::CreateAssetFile(root, asset, "second");

    AXIS_CHECK(created.success);
    AXIS_CHECK(!rejected.success);
    AXIS_CHECK(ReadText(asset) == "first");
    std::filesystem::remove_all(root, error);
}

AXIS_TEST_CASE("EditorFileService generates a unique duplicate name")
{
    const auto root = axis_test_support::TempPath("editor_file_service_duplicate");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    const auto asset = root / "material.axs";
    AXIS_CHECK(EditorFileService::CreateAssetFile(root, asset, "source").success);
    AXIS_CHECK(EditorFileService::CreateAssetFile(root, root / "material_copy.axs", "existing").success);

    const auto duplicate = EditorFileService::DuplicateFile(root, asset);

    AXIS_CHECK(duplicate.success);
    AXIS_CHECK(duplicate.path.filename() == "material_copy_2.axs");
    AXIS_CHECK(ReadText(duplicate.path) == "source");
    AXIS_CHECK(ReadText(root / "material_copy.axs") == "existing");
    std::filesystem::remove_all(root, error);
}

AXIS_TEST_CASE("EditorFileService rejects paths outside the project")
{
    const auto root = axis_test_support::TempPath("editor_file_service_root");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    const auto outside = root.parent_path() / "axis_editor_escape.txt";

    const auto result = EditorFileService::CreateAssetFile(root, outside, "blocked");

    AXIS_CHECK(!result.success);
    AXIS_CHECK(!std::filesystem::exists(outside));
    std::filesystem::remove_all(root, error);
}

AXIS_TEST_CASE("EditorFileService resolves directory symlinks before root checks")
{
    const auto root = axis_test_support::TempPath("editor_file_service_symlink_root");
    const auto outside = axis_test_support::TempPath("editor_file_service_symlink_outside");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::remove_all(outside, error);
    std::filesystem::create_directories(root);
    std::filesystem::create_directories(outside);
    const auto link = root / "escape";
    std::filesystem::create_directory_symlink(outside, link, error);
    if (!error)
    {
        const auto result = EditorFileService::CreateAssetFile(root, link / "blocked.txt", "blocked");
        AXIS_CHECK(!result.success);
        AXIS_CHECK(!std::filesystem::exists(outside / "blocked.txt"));
    }
    std::filesystem::remove_all(root, error);
    std::filesystem::remove_all(outside, error);
}

AXIS_TEST_CASE("EditorFileService rename refuses destination conflicts")
{
    const auto root = axis_test_support::TempPath("editor_file_service_rename");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    const auto source = root / "source.txt";
    const auto destination = root / "destination.txt";
    AXIS_CHECK(EditorFileService::CreateAssetFile(root, source, "source").success);
    AXIS_CHECK(EditorFileService::CreateAssetFile(root, destination, "destination").success);

    const auto result = EditorFileService::Rename(root, source, destination);

    AXIS_CHECK(!result.success);
    AXIS_CHECK(ReadText(source) == "source");
    AXIS_CHECK(ReadText(destination) == "destination");
    std::filesystem::remove_all(root, error);
}
#endif
