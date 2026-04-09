#include <core/logic/event_manager.h>

EventManager& EventManager::Instance() {
    static EventManager instance;
    return instance;
}
