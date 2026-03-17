#include <core/interface/i_loader_strategy.h>
#include <core/io/config_loader.h>
#include <platform/logic/input_system.h>
#include <platform/logic/io_handler.h>
#include <physics/io/physics_loader.h>
#include <scene/io/component_loader.h>
#include <platform/io/input_loader.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <fstream>
#include <sstream>

class ConfigLoaderStrategy : public ILoaderStrategy {
public:
    bool Load(const std::string& path, EngineContext ctx) override {
        std::ifstream file(FileSystem::getPath(path));
        if (!file.is_open()) return false;
        std::stringstream ss;
        ss << file.rdbuf();
        ConfigLoader::LoadConfig(ss, ctx);
        return true;
    }
    const char* GetName() const override { return "CONFIG"; }
};

class InputLoaderStrategy : public ILoaderStrategy {
public:
    bool Load(const std::string& path, EngineContext ctx) override {
        if (!ctx.io) return false;
        return InputLoader::LoadBindings(path, ctx.io->GetInputManager());
    }
    const char* GetName() const override { return "INPUT"; }
};

class PhysicsLoaderStrategy : public ILoaderStrategy {
public:
    bool Load(const std::string& path, EngineContext ctx) override {
        // Placeholder: PhysicsLoader usually operates on individual nodes during scene load.
        // This strategy could be expanded to load whole physics presets if needed.
        LOGGER_INFO("PhysicsLoaderStrategy") << "Physics strategy invoked for: " << path;
        return true; 
    }
    const char* GetName() const override { return "PHYSICS"; }
};

class ComponentLoaderStrategy : public ILoaderStrategy {
public:
    bool Load(const std::string& path, EngineContext ctx) override {
        // Similar to physics, ComponentLoader is usually called per-node.
        // We could use this to trigger global component registry initialization.
        ComponentLoader::InitializeDefaultLoaders();
        return true;
    }
    const char* GetName() const override { return "COMPONENT"; }
};

// Helper function to register all default strategies
#include <core/manager/unified_loader.h>
void RegisterDefaultLoaderStrategies() {
    auto& ul = UnifiedLoader::Instance();
    ul.Register(std::make_unique<ConfigLoaderStrategy>());
    ul.Register(std::make_unique<InputLoaderStrategy>());
    ul.Register(std::make_unique<PhysicsLoaderStrategy>());
    ul.Register(std::make_unique<ComponentLoaderStrategy>());
}
