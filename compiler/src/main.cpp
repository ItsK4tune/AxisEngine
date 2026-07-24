#include <axis_app.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/binary_scene_serializer.h>
#include <audio/logic/audio_service.h>
#include <physics/interface/i_physics_world.h>
#include <iostream>
#include <memory>
#include <string>

class CompilerApplication : public Application
{
public:
    void RegisterUserScripts() override
    {
    }
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file.axs> <output_file.axsb>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    auto app = std::make_shared<CompilerApplication>();

    AppConfig config;
    config.headlessMode = true;
    config.logLevel = LogLevel::None;

    if (!app->Initialize(config))
    {
        std::cerr << "Failed to initialize AxisEngine core for compilation." << std::endl;
        return 1;
    }

    Scene& scene = app->GetScene();
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    auto* physics = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    auto* audio = ServiceLocator::Instance().Resolve<AudioService>();

    SceneSerializer serializer(resources, physics, audio);
    SceneLoadResult res;
    if (!serializer.Deserialize(inputPath, scene, res))
    {
        std::cerr << "Failed to deserialize input scene file: " << inputPath << std::endl;
        app->Shutdown();
        return 1;
    }

    BinarySceneSerializer binSerializer;
    if (!binSerializer.Serialize(outputPath, scene))
    {
        std::cerr << "Failed to serialize binary scene file to: " << outputPath << std::endl;
        app->Shutdown();
        return 1;
    }

    std::cout << "Successfully compiled scene: " << inputPath << " -> " << outputPath << std::endl;
    app->Shutdown();
    return 0;
}
