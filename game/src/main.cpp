#include <core/application.h>

#include <states/game_state.h>
#include <iostream>

int main() {
    Application app;

    if (app.Init()) {
        app.PushState<GameState>();
        app.Run();
    }
    
    return 0;
}