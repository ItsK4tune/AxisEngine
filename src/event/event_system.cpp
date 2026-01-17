#include <engine/event/event_system.h>

EventSystem& EventSystem::Instance() {
    static EventSystem instance;
    return instance;
}
