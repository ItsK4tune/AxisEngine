#include <engine/core/event_system.h>

EventSystem& EventSystem::Instance() {
    static EventSystem instance;
    return instance;
}
