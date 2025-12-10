#include <engine/core/application.h>

#include <game/states/placeholder_state.h>
#include <iostream>

int main() {
    AppConfig config;
    config.title = "GameEngine";
    config.width = 1280;
    config.height = 720;

    Application app(config);

    if (app.Init()) {
        app.PushState<PlaceHolderState>();
        
        app.Run();
    }
    
    return 0;
}