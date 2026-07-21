#pragma once

#include <core/logic/service_locator.h>
#include <core/logic/event_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace axis_test_support
{
inline void ResetServices()
{
    ServiceLocator::Instance().ClearAll();
    EventManager::Instance().Clear();
}

inline std::filesystem::path TempPath(const std::string& filename)
{
    auto base = std::filesystem::temp_directory_path() / "axis_test";
    std::filesystem::create_directories(base);
    return base / filename;
}

inline std::filesystem::path WriteTempFile(const std::string& filename, const std::string& content)
{
    auto path = TempPath(filename);
    std::ofstream file(path);
    file << content;
    return path;
}

struct HeadlessResourceFixture
{
    HeadlessResourceFixture()
    {
        ResetServices();
        resources.InitializeHeadless();
        ServiceLocator::Instance().Register<ResourceManager>(&resources);
    }

    ~HeadlessResourceFixture()
    {
        resources.Shutdown();
        ResetServices();
    }

    ResourceManager resources;
};

struct SceneServiceFixture : HeadlessResourceFixture
{
    SceneServiceFixture()
    {
        ServiceLocator::Instance().Register<Scene>(&scene);
    }

    Scene scene;
};
}  // namespace axis_test_support
