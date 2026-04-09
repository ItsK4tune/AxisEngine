#include "test_framework.h"
#include <core/logic/event_manager.h>
#include <string>
#include <vector>

// --- Test event types ---

struct TestEvent {
    int value;
};

struct AnotherEvent {
    std::string message;
};

// --- Tests ---

TEST_CASE(EventManager_SubscribeAndPublish)
{
    auto& es = EventManager::Instance();
    int received = 0;

    int id = es.Subscribe<TestEvent>([&](const TestEvent& e) {
        received = e.value;
    });

    es.Publish(TestEvent{42});
    REQUIRE_EQUAL(received, 42);

    es.Unsubscribe<TestEvent>(id);
}

TEST_CASE(EventManager_MultipleSubscribers)
{
    auto& es = EventManager::Instance();
    std::vector<int> results;

    int id1 = es.Subscribe<TestEvent>([&](const TestEvent& e) {
        results.push_back(e.value);
    });
    int id2 = es.Subscribe<TestEvent>([&](const TestEvent& e) {
        results.push_back(e.value * 10);
    });

    es.Publish(TestEvent{5});
    REQUIRE_EQUAL((int)results.size(), 2);
    REQUIRE_EQUAL(results[0], 5);
    REQUIRE_EQUAL(results[1], 50);

    es.Unsubscribe<TestEvent>(id1);
    es.Unsubscribe<TestEvent>(id2);
}

TEST_CASE(EventManager_Unsubscribe)
{
    auto& es = EventManager::Instance();
    int count = 0;

    int id = es.Subscribe<TestEvent>([&](const TestEvent&) {
        count++;
    });

    es.Publish(TestEvent{1});
    REQUIRE_EQUAL(count, 1);

    es.Unsubscribe<TestEvent>(id);

    es.Publish(TestEvent{2});
    REQUIRE_EQUAL(count, 1); // Should not increment
}

TEST_CASE(EventManager_DifferentEventTypes)
{
    auto& es = EventManager::Instance();
    int intResult = 0;
    std::string strResult;

    int id1 = es.Subscribe<TestEvent>([&](const TestEvent& e) {
        intResult = e.value;
    });
    int id2 = es.Subscribe<AnotherEvent>([&](const AnotherEvent& e) {
        strResult = e.message;
    });

    es.Publish(TestEvent{99});
    es.Publish(AnotherEvent{"hello"});

    REQUIRE_EQUAL(intResult, 99);
    REQUIRE_EQUAL(strResult, std::string("hello"));

    es.Unsubscribe<TestEvent>(id1);
    es.Unsubscribe<AnotherEvent>(id2);
}

TEST_CASE(EventManager_ScopedSubscriber)
{
    auto& es = EventManager::Instance();
    int count = 0;

    {
        ScopedSubscriber<TestEvent> sub(
            es.Subscribe<TestEvent>([&](const TestEvent&) { count++; })
        );
        es.Publish(TestEvent{1});
        REQUIRE_EQUAL(count, 1);
    } // ScopedSubscriber goes out of scope, auto-unsubscribes

    es.Publish(TestEvent{2});
    REQUIRE_EQUAL(count, 1); // Should not increment
}

TEST_CASE(EventManager_ScopedSubscriber_MoveSemantics)
{
    auto& es = EventManager::Instance();
    int count = 0;

    ScopedSubscriber<TestEvent> sub1(
        es.Subscribe<TestEvent>([&](const TestEvent&) { count++; })
    );

    ScopedSubscriber<TestEvent> sub2(std::move(sub1));

    es.Publish(TestEvent{1});
    REQUIRE_EQUAL(count, 1);
}
