#include <editor/logic/editor_file_service.h>

#ifdef ENABLE_EDITOR
#include <algorithm>
#include <cctype>
#include <limits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace
{
std::filesystem::path NormalizeExistingParent(const std::filesystem::path& path, std::error_code& error)
{
    if (std::filesystem::exists(path, error))
        return error ? std::filesystem::path{} : std::filesystem::weakly_canonical(path, error);
    if (error)
        return {};
    const std::filesystem::path parent = path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
    const std::filesystem::path normalizedParent = std::filesystem::weakly_canonical(parent, error);
    return error ? std::filesystem::path{} : (normalizedParent / path.filename()).lexically_normal();
}

bool ComponentsEqual(const std::filesystem::path& left, const std::filesystem::path& right)
{
#ifdef _WIN32
    std::string lhs = left.generic_string();
    std::string rhs = right.generic_string();
    std::transform(lhs.begin(), lhs.end(), lhs.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    std::transform(rhs.begin(), rhs.end(), rhs.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return lhs == rhs;
#else
    return left == right;
#endif
}

EditorFileResult Rejected(const std::string& message)
{
    return {false, {}, message};
}

bool WriteNewFile(const std::filesystem::path& path, std::string_view content, std::string& errorMessage)
{
#ifdef _WIN32
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        errorMessage = GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS
                           ? "A file with that name already exists."
                           : "Could not create asset.";
        return false;
    }

    size_t offset = 0;
    bool success = true;
    while (offset < content.size())
    {
        const DWORD chunk = static_cast<DWORD>((std::min)(
            content.size() - offset, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
        DWORD written = 0;
        if (!WriteFile(file, content.data() + offset, chunk, &written, nullptr) || written != chunk)
        {
            success = false;
            break;
        }
        offset += written;
    }
    success = success && FlushFileBuffers(file) != 0;
    CloseHandle(file);
    if (!success)
    {
        DeleteFileW(path.c_str());
        errorMessage = "Could not finish writing the asset.";
    }
    return success;
#else
    const int file = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (file < 0)
    {
        errorMessage = errno == EEXIST ? "A file with that name already exists." : std::strerror(errno);
        return false;
    }

    size_t offset = 0;
    bool success = true;
    while (offset < content.size())
    {
        const ssize_t written = write(file, content.data() + offset, content.size() - offset);
        if (written <= 0)
        {
            success = false;
            break;
        }
        offset += static_cast<size_t>(written);
    }
    success = success && fsync(file) == 0;
    close(file);
    if (!success)
    {
        unlink(path.c_str());
        errorMessage = "Could not finish writing the asset.";
    }
    return success;
#endif
}
}

bool EditorFileService::IsWithinRoot(const std::filesystem::path& root, const std::filesystem::path& target)
{
    std::error_code error;
    const std::filesystem::path normalizedRoot = std::filesystem::weakly_canonical(root, error);
    if (error)
        return false;
    const std::filesystem::path normalizedTarget = NormalizeExistingParent(target, error);
    if (error)
        return false;

    auto rootPart = normalizedRoot.begin();
    auto targetPart = normalizedTarget.begin();
    for (; rootPart != normalizedRoot.end(); ++rootPart, ++targetPart)
    {
        if (targetPart == normalizedTarget.end() || !ComponentsEqual(*rootPart, *targetPart))
            return false;
    }
    return true;
}

EditorFileResult EditorFileService::CreateProjectDirectory(const std::filesystem::path& root,
                                                           const std::filesystem::path& path)
{
    if (!IsWithinRoot(root, path))
        return Rejected("Path is outside the project root.");

    std::error_code error;
    const bool created = std::filesystem::create_directory(path, error);
    if (error)
        return Rejected(error.message());
    if (!created)
        return Rejected("A file or folder with that name already exists.");
    return {true, path, "Folder created."};
}

EditorFileResult EditorFileService::CreateAssetFile(const std::filesystem::path& root,
                                                    const std::filesystem::path& path,
                                                    std::string_view content)
{
    if (!IsWithinRoot(root, path))
        return Rejected("Path is outside the project root.");

    std::string errorMessage;
    if (!WriteNewFile(path, content, errorMessage))
        return Rejected(errorMessage);
    return {true, path, "Asset created."};
}

EditorFileResult EditorFileService::DuplicateFile(const std::filesystem::path& root,
                                                  const std::filesystem::path& source)
{
    if (!IsWithinRoot(root, source))
        return Rejected("Source is outside the project root.");

    std::error_code error;
    if (!std::filesystem::is_regular_file(source, error) || error)
        return Rejected(error ? error.message() : "Only files can be duplicated.");

    for (uint32_t suffix = 1; suffix < 100000; ++suffix)
    {
        const std::string marker = suffix == 1 ? "_copy" : "_copy_" + std::to_string(suffix);
        const std::filesystem::path destination =
            source.parent_path() / (source.stem().string() + marker + source.extension().string());
        if (!IsWithinRoot(root, destination))
            return Rejected("Destination is outside the project root.");
        if (std::filesystem::exists(destination, error))
        {
            if (error)
                return Rejected(error.message());
            continue;
        }
        if (std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, error))
            return {true, destination, "Asset duplicated."};
        if (error)
            return Rejected(error.message());
    }
    return Rejected("Could not find an available duplicate name.");
}

EditorFileResult EditorFileService::Rename(const std::filesystem::path& root,
                                           const std::filesystem::path& source,
                                           const std::filesystem::path& destination)
{
    if (!IsWithinRoot(root, source) || !IsWithinRoot(root, destination))
        return Rejected("Path is outside the project root.");

    std::error_code error;
    if (std::filesystem::exists(destination, error) || error)
        return Rejected(error ? error.message() : "A file or folder with that name already exists.");
    std::filesystem::rename(source, destination, error);
    return error ? Rejected(error.message()) : EditorFileResult{true, destination, "Asset renamed."};
}

EditorFileResult EditorFileService::Remove(const std::filesystem::path& root,
                                           const std::filesystem::path& path)
{
    if (!IsWithinRoot(root, path))
        return Rejected("Path is outside the project root.");

    std::error_code error;
    const bool removed = std::filesystem::remove(path, error);
    if (error)
        return Rejected(error.message());
    return removed ? EditorFileResult{true, {}, "Asset deleted."}
                   : Rejected("Nothing was deleted.");
}
#endif
