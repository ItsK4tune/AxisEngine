#pragma once

#include <memory>

class IPlatformFileSystem;
class IPlatformRuntime;

std::shared_ptr<IPlatformFileSystem> AcquirePlatformFileSystem();
bool SetPlatformFileSystemProvider(std::shared_ptr<IPlatformFileSystem> provider);
void ResetPlatformFileSystemProvider();

std::shared_ptr<IPlatformRuntime> AcquirePlatformRuntime();
bool SetPlatformRuntimeProvider(std::shared_ptr<IPlatformRuntime> provider);
void ResetPlatformRuntimeProvider();
