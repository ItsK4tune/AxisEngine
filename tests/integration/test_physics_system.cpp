#include "mocks/fake_physics.h"
#include "test_framework.h"
#include "test_support.h"

#include <core/logic/service_locator.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/logic/physics_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <physics/logic/physics_loader.h>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <core/logic/yaml_parser.h>
#include <glm/gtc/matrix_transform.hpp>

using axis_test_mocks::FakePhysicsWorld;

namespace
{
YAMLNode SingleNode(const std::string& content)
{
    auto nodes = YAMLParser::ParseString(content);
    AXIS_CHECK(nodes.size() == 1);
    return nodes[0];
}

bool HasShapeCall(const FakePhysicsWorld& physics, const std::string& type)
{
    for (const auto& call : physics.shapeCalls)
    {
        if (call.type == type)
            return true;
    }
    return false;
}

class BulletBodyRegistration
{
public:
    explicit BulletBodyRegistration(BulletPhysicsWorld& world) : m_World(world)
    {
    }

    ~BulletBodyRegistration()
    {
        for (auto it = m_Bodies.rbegin(); it != m_Bodies.rend(); ++it)
            m_World.RemoveRigidBody(*it);
    }

    void Add(IRigidBody* body)
    {
        m_World.AddRigidBody(body);
        m_Bodies.push_back(body);
    }

private:
    BulletPhysicsWorld& m_World;
    std::vector<IRigidBody*> m_Bodies;
};
}  // namespace

AXIS_TEST_CASE("PhysicsLoader LoadRigidShape parses primitive shape fields")
{
    Scene scene;
    FakePhysicsWorld physics;
    auto entity = EntityManager::CreateEntity(scene, "Body");
    auto node = SingleNode(
        "RigidShape:\n"
        "  Type: BOX\n"
        "  Size: 1 2 3\n"
        "  Radius: 0.75\n"
        "  Height: 2.5\n"
        "  Offset: 4 5 6\n"
        "  Rotation: 0 90 0\n"
        "  Friction: 0.8\n"
        "  Restitution: 0.25\n");

    PhysicsLoader::LoadRigidShape(scene, entity, node, physics);

    const auto& shape = scene.registry.get<RigidShapeComponent>(entity);
    AXIS_CHECK(shape.type == ShapeType::Box);
    AXIS_CHECK_NEAR(shape.size.x, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(shape.size.y, 2.0f, 0.0001f);
    AXIS_CHECK_NEAR(shape.size.z, 3.0f, 0.0001f);
    AXIS_CHECK_NEAR(shape.radius, 0.75f, 0.0001f);
    AXIS_CHECK_NEAR(shape.height, 2.5f, 0.0001f);
    AXIS_CHECK_NEAR(shape.offset.x, 4.0f, 0.0001f);
    AXIS_CHECK_NEAR(shape.friction, 0.8f, 0.0001f);
    AXIS_CHECK_NEAR(shape.restitution, 0.25f, 0.0001f);
}

AXIS_TEST_CASE("PhysicsLoader legacy RigidBody with Type creates shape and body components")
{
    Scene scene;
    FakePhysicsWorld physics;
    auto entity = EntityManager::CreateEntity(scene, "Body");
    auto node = SingleNode(
        "RigidBody:\n"
        "  Type: BOX\n"
        "  Size: 1 2 3\n"
        "  Mass: 2\n"
        "  BodyType: DYNAMIC\n"
        "  LinearFactor: 1 0 1\n"
        "  AngularFactor: 0 1 0\n"
        "  LinearDamping: 0.2\n"
        "  AngularDamping: 0.3\n"
        "  CollisionEnabled: false\n");

    PhysicsLoader::LoadRigidBody(scene, entity, node, physics);

    AXIS_CHECK(scene.registry.all_of<RigidShapeComponent>(entity));
    AXIS_CHECK(scene.registry.all_of<RigidBodyComponent>(entity));
    const auto& shape = scene.registry.get<RigidShapeComponent>(entity);
    const auto& body = scene.registry.get<RigidBodyComponent>(entity);
    AXIS_CHECK(shape.type == ShapeType::Box);
    AXIS_CHECK_NEAR(body.mass, 2.0f, 0.0001f);
    AXIS_CHECK(!body.isStatic);
    AXIS_CHECK(!body.isCollisionEnabled);
    AXIS_CHECK_NEAR(body.linearFactor.y, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(body.angularDamping, 0.3f, 0.0001f);
}

AXIS_TEST_CASE("PhysicsSystem auto-adds RigidBodyComponent for shape-only entity")
{
    axis_test_support::ResetServices();
    Scene scene;
    FakePhysicsWorld physics;
    ServiceLocator::Instance().Register<IPhysicsWorld>(&physics);
    PhysicsSystem system;
    auto entity = EntityManager::CreateEntity(scene, "ShapeOnly");
    auto& shape = scene.registry.emplace<RigidShapeComponent>(entity);
    shape.type = ShapeType::Box;

    system.FixedUpdate(scene, 1.0f / 60.0f);

    AXIS_CHECK(scene.registry.all_of<RigidBodyComponent>(entity));
    AXIS_CHECK(scene.registry.get<RigidBodyComponent>(entity).body != nullptr);
    AXIS_CHECK(physics.rigidBodies.size() == 1);
    system.Reset();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("PhysicsSystem InitializeRigidBodyDirect creates primitive shapes")
{
    Scene scene;
    FakePhysicsWorld physics;
    PhysicsSystem system;
    const ShapeType types[] = {ShapeType::Box, ShapeType::Sphere, ShapeType::Capsule, ShapeType::Cylinder};

    for (ShapeType type : types)
    {
        auto entity = EntityManager::CreateEntity(scene, "Primitive");
        auto& shape = scene.registry.emplace<RigidShapeComponent>(entity);
        auto& body = scene.registry.emplace<RigidBodyComponent>(entity);
        shape.type = type;
        shape.size = {1.0f, 2.0f, 3.0f};
        shape.radius = 0.5f;
        shape.height = 2.0f;
        system.InitializeRigidBodyDirect(scene, entity, shape, body, physics);
        AXIS_CHECK(body.body != nullptr);
    }

    AXIS_CHECK(HasShapeCall(physics, "Box"));
    AXIS_CHECK(HasShapeCall(physics, "Sphere"));
    AXIS_CHECK(HasShapeCall(physics, "Capsule"));
    AXIS_CHECK(HasShapeCall(physics, "Cylinder"));
}

AXIS_TEST_CASE("PhysicsSystem wraps offset shape in compound")
{
    Scene scene;
    FakePhysicsWorld physics;
    PhysicsSystem system;
    auto entity = EntityManager::CreateEntity(scene, "OffsetBody");
    auto& shape = scene.registry.emplace<RigidShapeComponent>(entity);
    auto& body = scene.registry.emplace<RigidBodyComponent>(entity);
    shape.type = ShapeType::Box;
    shape.offset = {1.0f, 0.0f, 0.0f};

    system.InitializeRigidBodyDirect(scene, entity, shape, body, physics);

    AXIS_CHECK(body.body != nullptr);
    AXIS_CHECK(HasShapeCall(physics, "Compound"));
    AXIS_CHECK(physics.childShapeCalls.size() == 1);
    AXIS_CHECK_NEAR(physics.childShapeCalls[0].position.x, 1.0f, 0.0001f);
    AXIS_CHECK(physics.lastRigidBodyShape->GetType() == CollisionShapeType::CompoundHull);
}

AXIS_TEST_CASE("PhysicsSystem collision filter respects disabled collision flag")
{
    axis_test_support::ResetServices();
    Scene scene;
    FakePhysicsWorld physics;
    ServiceLocator::Instance().Register<IPhysicsWorld>(&physics);
    PhysicsSystem system;
    auto enabled = EntityManager::CreateEntity(scene, "Enabled");
    auto disabled = EntityManager::CreateEntity(scene, "Disabled");
    scene.registry.emplace<RigidBodyComponent>(enabled);
    auto& disabledBody = scene.registry.emplace<RigidBodyComponent>(disabled);
    disabledBody.isCollisionEnabled = false;

    system.FixedUpdate(scene, 1.0f / 60.0f);

    AXIS_CHECK(physics.collisionFilter != nullptr);
    AXIS_CHECK(!physics.collisionFilter(enabled, disabled));
    AXIS_CHECK(!physics.collisionFilter(disabled, enabled));
    system.Reset();
    axis_test_support::ResetServices();
}

AXIS_TEST_CASE("BulletPhysicsWorld raycast hits nearest body and honors ignore entity")
{
    BulletPhysicsWorld world;
    world.Initialize();
    world.SetGravity(glm::vec3(0.0f));
    auto shape = world.CreateBoxShape({0.5f, 0.5f, 0.5f});
    auto top = world.CreateRigidBody(0.0f, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), shape);
    auto bottom = world.CreateRigidBody(0.0f, {0.0f, -3.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), shape);
    const auto topEntity = static_cast<entt::entity>(1);
    const auto bottomEntity = static_cast<entt::entity>(2);
    top->SetUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(topEntity) + 1));
    bottom->SetUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(bottomEntity) + 1));
    BulletBodyRegistration bodies(world);
    bodies.Add(top.get());
    bodies.Add(bottom.get());

    auto firstHit = world.Raycast({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 10.0f);
    auto ignoredHit = world.Raycast({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 10.0f, topEntity);

    AXIS_CHECK(firstHit.hasHit);
    AXIS_CHECK(firstHit.entity == topEntity);
    AXIS_CHECK(ignoredHit.hasHit);
    AXIS_CHECK(ignoredHit.entity == bottomEntity);
}

AXIS_TEST_CASE("BulletPhysicsWorld collects normal and trigger collision info")
{
    BulletPhysicsWorld world;
    world.Initialize();
    world.SetGravity(glm::vec3(0.0f));
    auto shape = world.CreateBoxShape({1.0f, 1.0f, 1.0f});
    auto bodyA = world.CreateRigidBody(0.0f, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), shape);
    auto bodyB = world.CreateRigidBody(1.0f, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), shape);
    const auto entityA = static_cast<entt::entity>(3);
    const auto entityB = static_cast<entt::entity>(4);
    bodyA->SetUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entityA) + 1));
    bodyB->SetUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entityB) + 1));
    BulletBodyRegistration bodies(world);
    bodies.Add(bodyA.get());
    bodies.Add(bodyB.get());

    world.Update(1.0f / 60.0f);
    std::vector<CollisionInfo> collisions;
    world.CollectActiveCollisions(collisions);
    AXIS_CHECK(!collisions.empty());
    AXIS_CHECK(!collisions[0].isTrigger);

    bodyB->SetTrigger(true);
    world.Update(1.0f / 60.0f);
    world.CollectActiveCollisions(collisions);
    AXIS_CHECK(!collisions.empty());
    bool sawTrigger = false;
    for (const auto& collision : collisions)
    {
        if ((collision.bodyA == entityA && collision.bodyB == entityB) ||
            (collision.bodyA == entityB && collision.bodyB == entityA))
        {
            sawTrigger = sawTrigger || collision.isTrigger;
        }
    }
    AXIS_CHECK(sawTrigger);
}
