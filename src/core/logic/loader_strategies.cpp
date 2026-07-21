#include <core/interface/i_loader_strategy.h>
#include <core/logic/loader_strategies.h>
#include <core/logic/config_manager.h>
#include <core/logic/config_serializer.h>
#include <core/logic/filesystem.h>
#include <core/logic/service_locator.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>

class ConfigLoaderStrategy : public ILoaderStrategy
{
public:
    bool Load(const std::string& path) override
    {
        auto& configMgr = ServiceLocator::Instance().Require<ConfigManager>();
        AppConfig temp = configMgr.GetConfig();
        ConfigSerializer serializer(configMgr.IsHeadless());
        if (!serializer.Deserialize(FileSystem::getPath(path), temp))
            return false;
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
        if (auto* io = sl.Resolve<IOHandler>())
        {
            InputSerializer serializer;
            return serializer.Deserialize(path, io->GetInputManager());
        }
        return false;
    }
    const char* GetName() const override
    {
        return "INPUT";
    }
};

#include <resource/logic/resource_manager.h>

void RegisterDefaultLoaderStrategies(ResourceManager& loader)
{
    loader.RegisterLoaderInternal(std::make_unique<ConfigLoaderStrategy>(), false);
    loader.RegisterLoaderInternal(std::make_unique<InputLoaderStrategy>(), false);
}
