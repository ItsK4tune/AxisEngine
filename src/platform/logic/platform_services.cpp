#include <platform/logic/platform_services.h>
#include <platform/interface/i_platform_filesystem.h>
#include <platform/interface/i_platform_runtime.h>

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

std::unique_ptr<IPlatformFileSystem> CreateNativePlatformFileSystem()
{
#if defined(_WIN32)
    return std::make_unique<WindowsFileSystem>();
#elif defined(__APPLE__)
    return std::make_unique<MacFileSystem>();
#else
    return std::make_unique<PosixFileSystem>();
#endif
}

IPlatformFileSystem& GetNativePlatformFileSystem()
{
    static std::unique_ptr<IPlatformFileSystem> filesystem = CreateNativePlatformFileSystem();
    return *filesystem;
}

std::unique_ptr<IPlatformRuntime> CreateNativePlatformRuntime()
{
#if defined(_WIN32)
    return std::make_unique<WindowsRuntime>();
#elif defined(__APPLE__)
    return std::make_unique<MacRuntime>();
#else
    return std::make_unique<PosixRuntime>();
#endif
}

IPlatformRuntime& GetNativePlatformRuntime()
{
    static std::unique_ptr<IPlatformRuntime> runtime = CreateNativePlatformRuntime();
    return *runtime;
}
