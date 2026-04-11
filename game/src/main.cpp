#include <axis_app.h>
#include <states/game_state.h>

int main() {
    auto app = std::make_shared<Application>();

    AppConfig config;
    config.title = "Axis Engine - Game";
    config.width = 1280;
    config.height = 720;
    config.logLevel = LogLevel::Verbose;
    config.antialiasing = 2;

    if (app->Initialize(config)) {
        app->PushState<GameState>();
        app->Run();
    }

    return 0;
}
