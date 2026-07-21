#include <axis_app.h>
#ifdef ENABLE_EDITOR
#include <editor/editor_system.h>
#endif
#include "sample_state.h"

class SampleApplication : public Application
{
public:
#ifdef ENABLE_EDITOR
    void RegisterUserSystems(ISystemRegistry& systems) override
    {
        systems.RegisterSystem(std::make_unique<EditorSystem>());
    }
#endif

    void RegisterUserScripts() override
    {
        // Script registry requires registering the script classes
        RegisterScript<OrbitScript>("OrbitScript");
        RegisterScript<PulseScaleScript>("PulseScaleScript");
        RegisterScript<ColorShiftScript>("ColorShiftScript");
        RegisterScript<RandomMoveScript>("RandomMoveScript");
        RegisterScript<RotateScript>("RotateScript");
        RegisterScript<BouncingScript>("BouncingScript");
        RegisterScript<PlayerControlScript>("PlayerControlScript");
        RegisterScript<CharacterControllerDemoScript>("CharacterControllerDemoScript");
        RegisterScript<Scenario26CharacterControllerScript>("Scenario26CharacterControllerScript");
        RegisterScript<Scenario26FpsCameraScript>("Scenario26FpsCameraScript");
        RegisterScript<CollisionReporterScript>("CollisionReporterScript");
    }
};

int main()
{
    auto app = std::make_shared<SampleApplication>();

    AppConfig config;
    config.title = "AxisEngine Benchmarking Samples";
    config.window.width = 1600;
    config.window.height = 900;
    config.logLevel = LogLevel::Verbose;
    config.graphics.antialiasing = 2;
    config.headlessMode = false;

    if (app->Initialize(config))
    {
        app->PushState<SampleState>();
        app->Run();
    }

    return 0;
}
