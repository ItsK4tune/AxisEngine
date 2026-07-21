#include <platform/logic/platform_services.h>
#include <platform/interface/i_platform_filesystem.h>
#include <platform/interface/i_platform_runtime.h>
#include <mutex>

#if defined(_WIN32)
#include <platform/strategy/windows/windows_filesystem.h>
#include <platform/strategy/windows/windows_runtime.h>
#elif defined(__APPLE__)
#include <platform/strategy/macos/macos_filesystem.h>
#include <platform/strategy/macos/macos_runtime.h>
#else
#include <platform/strategy/posix/posix_filesystem.h>
#include <platform/strategy/posix/posix_runtime.h>
#endif

namespace
{
std::mutex s_PlatformServicesMutex;
std::shared_ptr<IPlatformFileSystem> s_FileSystem;
std::shared_ptr<IPlatformRuntime> s_Runtime;

std::shared_ptr<IPlatformFileSystem> CreateNativePlatformFileSystem()
{
#if defined(_WIN32)
    return std::make_shared<WindowsFileSystem>();
#elif defined(__APPLE__)
    return std::make_shared<MacFileSystem>();
#else
    return std::make_shared<PosixFileSystem>();
#endif
}

std::shared_ptr<IPlatformRuntime> CreateNativePlatformRuntime()
{
#if defined(_WIN32)
    return std::make_shared<WindowsRuntime>();
#elif defined(__APPLE__)
    return std::make_shared<MacRuntime>();
#else
    return std::make_shared<PosixRuntime>();
#endif
}
}  // namespace

std::shared_ptr<IPlatformFileSystem> AcquirePlatformFileSystem()
{
    std::lock_guard lock(s_PlatformServicesMutex);
    if (!s_FileSystem)
        s_FileSystem = CreateNativePlatformFileSystem();
    return s_FileSystem;
}

bool SetPlatformFileSystemProvider(std::shared_ptr<IPlatformFileSystem> provider)
{
    if (!provider)
        return false;
    std::lock_guard lock(s_PlatformServicesMutex);
    s_FileSystem = std::move(provider);
    return true;
}

void ResetPlatformFileSystemProvider()
{
    std::lock_guard lock(s_PlatformServicesMutex);
    s_FileSystem = CreateNativePlatformFileSystem();
}

std::shared_ptr<IPlatformRuntime> AcquirePlatformRuntime()
{
    std::lock_guard lock(s_PlatformServicesMutex);
    if (!s_Runtime)
        s_Runtime = CreateNativePlatformRuntime();
    return s_Runtime;
}

bool SetPlatformRuntimeProvider(std::shared_ptr<IPlatformRuntime> provider)
{
    if (!provider)
        return false;
    std::lock_guard lock(s_PlatformServicesMutex);
    s_Runtime = std::move(provider);
    return true;
}

void ResetPlatformRuntimeProvider()
{
    std::lock_guard lock(s_PlatformServicesMutex);
    s_Runtime = CreateNativePlatformRuntime();
}
