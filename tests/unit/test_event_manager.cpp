#include "test_framework.h"

#include <core/logic/event_manager.h>

namespace
{
struct EventManagerTestEventA
{
    int value = 0;
};

struct EventManagerTestEventB
{
    int value = 0;
};
}  // namespace

AXIS_TEST_CASE("EventManager generic unsubscribe removes listener")
{
    int calls = 0;
    auto& events = EventManager::Instance();

    int listenerId =
        events.Subscribe<EventManagerTestEventA>([&](const EventManagerTestEventA& e) { calls += e.value; });

    events.Publish(EventManagerTestEventA{2});
    AXIS_CHECK(calls == 2);

    events.Unsubscribe(listenerId);
    events.Publish(EventManagerTestEventA{3});
    AXIS_CHECK(calls == 2);
}

AXIS_TEST_CASE("EventSubscriptionList clears mixed event types")
{
    int calls = 0;
    auto& events = EventManager::Instance();

    {
        EventSubscriptionList subscriptions;
        subscriptions.Add(
            events.Subscribe<EventManagerTestEventA>([&](const EventManagerTestEventA& e) { calls += e.value; }));
        subscriptions.Add(
            events.Subscribe<EventManagerTestEventB>([&](const EventManagerTestEventB& e) { calls += e.value; }));

        events.Publish(EventManagerTestEventA{1});
        events.Publish(EventManagerTestEventB{2});
        AXIS_CHECK(calls == 3);
    }

    events.Publish(EventManagerTestEventA{4});
    events.Publish(EventManagerTestEventB{8});
    AXIS_CHECK(calls == 3);
}
