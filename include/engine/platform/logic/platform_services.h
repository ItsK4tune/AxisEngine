#pragma once

#include <memory>

class IPlatformFileSystem;
class IPlatformRuntime;

std::unique_ptr<IPlatformFileSystem> CreateNativePlatformFileSystem();
IPlatformFileSystem& GetNativePlatformFileSystem();

std::unique_ptr<IPlatformRuntime> CreateNativePlatformRuntime();
IPlatformRuntime& GetNativePlatformRuntime();
