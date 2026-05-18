#include <core/logic/service_locator.h>
#include "test_framework.h"

// --- Mock services for testing ---

struct MockRenderer
{
    int drawCalls = 0;
    void Draw()
    {
        drawCalls++;
    }
};

struct MockPhysics
{
    bool running = false;
    void Step()
    {
        running = true;
    }
};

struct MockAudio
{
    float volume = 1.0f;
};

// --- Tests ---

TEST_CASE(ServiceLocator_RegisterAndResolve)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    MockRenderer renderer;
    sl.Register<MockRenderer>(&renderer);

    auto* resolved = sl.Resolve<MockRenderer>();
    REQUIRE_NOT_NULL(resolved);
    REQUIRE_EQUAL(resolved, &renderer);

    resolved->Draw();
    REQUIRE_EQUAL(renderer.drawCalls, 1);

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_ResolveUnregistered_ReturnsNull)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    auto* resolved = sl.Resolve<MockPhysics>();
    REQUIRE_NULL(resolved);

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_Require_ThrowsIfMissing)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    REQUIRE_THROWS(sl.Require<MockPhysics>());

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_Require_ReturnsReference)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    MockPhysics physics;
    sl.Register<MockPhysics>(&physics);

    MockPhysics& ref = sl.Require<MockPhysics>();
    ref.Step();
    REQUIRE(physics.running);

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_Unregister)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    MockRenderer renderer;
    sl.Register<MockRenderer>(&renderer);
    REQUIRE(sl.Has<MockRenderer>());

    sl.Unregister<MockRenderer>();
    REQUIRE(!sl.Has<MockRenderer>());
    REQUIRE_NULL(sl.Resolve<MockRenderer>());

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_MultipleServices)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    MockRenderer renderer;
    MockPhysics physics;
    MockAudio audio;

    sl.Register<MockRenderer>(&renderer);
    sl.Register<MockPhysics>(&physics);
    sl.Register<MockAudio>(&audio);

    REQUIRE(sl.Has<MockRenderer>());
    REQUIRE(sl.Has<MockPhysics>());
    REQUIRE(sl.Has<MockAudio>());

    REQUIRE_EQUAL(sl.Resolve<MockRenderer>(), &renderer);
    REQUIRE_EQUAL(sl.Resolve<MockPhysics>(), &physics);
    REQUIRE_EQUAL(sl.Resolve<MockAudio>(), &audio);

    sl.ClearAll();
    REQUIRE(!sl.Has<MockRenderer>());
    REQUIRE(!sl.Has<MockPhysics>());
    REQUIRE(!sl.Has<MockAudio>());
}

TEST_CASE(ServiceLocator_ReplaceService)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    MockRenderer renderer1;
    renderer1.drawCalls = 10;
    MockRenderer renderer2;
    renderer2.drawCalls = 20;

    sl.Register<MockRenderer>(&renderer1);
    REQUIRE_EQUAL(sl.Resolve<MockRenderer>()->drawCalls, 10);

    sl.Register<MockRenderer>(&renderer2);
    REQUIRE_EQUAL(sl.Resolve<MockRenderer>()->drawCalls, 20);

    sl.ClearAll();
}

TEST_CASE(ServiceLocator_Has)
{
    auto& sl = ServiceLocator::Instance();
    sl.ClearAll();

    REQUIRE(!sl.Has<MockRenderer>());

    MockRenderer renderer;
    sl.Register<MockRenderer>(&renderer);
    REQUIRE(sl.Has<MockRenderer>());

    sl.ClearAll();
}
