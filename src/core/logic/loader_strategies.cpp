#include <core/interface/i_loader_strategy.h>
#include <core/logic/config_loader.h>
#include <core/logic/config_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <physics/logic/physics_loader.h>
#include <platform/logic/input_loader.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <scene/logic/component_loader.h>
#include <fstream>
#include <sstream>

class ConfigLoaderStrategy : public ILoaderStrategy
{
public:
    bool Load(const std::string& path) override
    {
        std::ifstream file(FileSystem::getPath(path));
        if (!file.is_open())
            return false;
        std::stringstream ss;
        ss << file.rdbuf();

        auto& configMgr = ServiceLocator::Instance().Require<ConfigManager>();
        AppConfig temp = configMgr.GetConfig();
        ConfigLoader::LoadConfig(ss, temp, configMgr.IsHeadless());
        configMgr.UpdateConfig(temp);
        return true;
    }
    const char* GetName() const override
    {
        return "CONFIG";
    }
};

class InputLoaderStrategy : public ILoaderStrategy
{
public:
    bool Load(const std::string& path) override
    {
        auto& sl = ServiceLocator::Instance();
        auto& input = sl.Require<InputManager>();
        return InputLoader::LoadBindings(FileSystem::getPath(path), input);
    }
    const char* GetName() const override
    {
        return "INPUT";
    }
};

class PhysicsLoaderStrategy : public ILoaderStrategy
{
public:
    bool Load(const std::string& path) override
    {
        LOGGER_INFO("PhysicsLoaderStrategy") << "Physics strategy invoked for: " << path;
        return true;
    }
    const char* GetName() const override
    {
        return "PHYSICS";
    }
};

class ComponentLoaderStrategy : public ILoaderStrategy
{
public:
    bool Load(const std::string& path) override
    {
        ComponentLoader::InitializeDefaultLoaders();
        return true;
    }
    const char* GetName() const override
    {
        return "COMPONENT";
    }
};

#include <resource/logic/resource_manager.h>

void RegisterDefaultLoaderStrategies()
{
    auto& loader = ServiceLocator::Instance().Require<ResourceManager>();
    loader.RegisterLoader(std::make_unique<ConfigLoaderStrategy>());
    loader.RegisterLoader(std::make_unique<InputLoaderStrategy>());
    loader.RegisterLoader(std::make_unique<PhysicsLoaderStrategy>());
    loader.RegisterLoader(std::make_unique<ComponentLoaderStrategy>());
}
