#include <axis_app.h>
#include "sample_state.h"

class SampleApplication : public Application
{
public:
    void RegisterUserScripts() override
    {
        // Script registry requires registering the script classes
        RegisterScript<OrbitScript>("OrbitScript");
        RegisterScript<PulseScaleScript>("PulseScaleScript");
        RegisterScript<ColorShiftScript>("ColorShiftScript");
    }
};

int main()
{
    auto app = std::make_shared<SampleApplication>();

    AppConfig config;
    config.title = "AxisEngine Benchmarking Samples";
    config.width = 1600;
    config.height = 900;
    config.logLevel = LogLevel::Verbose;
    config.antialiasing = 2;
    config.headlessMode = false;

    if (app->Initialize(config))
    {
        app->PushState<SampleState>();
        app->Run();
    }

    return 0;
}
