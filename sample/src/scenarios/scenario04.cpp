#include "sample_scenario_common.h"

void SampleState::LoadScene4()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(m_S4Gravity);
    }

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .Build();

    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floor);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(80.0f, 1.0f, 80.0f);
    floorShape.restitution = m_S4Restitution;
    floorShape.friction = m_S4Friction;

    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floor);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    std::string modelName = "cubeModel";
    ShapeType st = ShapeType::Box;
    if (m_S4ShapeType == 1) { modelName = "sphereModel"; st = ShapeType::Sphere; }
    else if (m_S4ShapeType == 2) { modelName = "capsuleModel"; st = ShapeType::Capsule; }

    for (int i = 0; i < m_S4EntityCount; ++i)
    {
        float x = static_cast<float>((i % 10) - 5) * m_S4GridSpacing + (static_cast<float>(rand() % 100) / 400.0f - 0.125f);
        float y = static_cast<float>(i / 25) * m_S4GridSpacing + m_S4SpawnHeight;
        float z = static_cast<float>(((i / 10) % 10) - 5) * m_S4GridSpacing + (static_cast<float>(rand() % 100) / 400.0f - 0.125f);

        auto bodyEntity = EntityBuilder(scene, res, "scenario")
            .WithName("PhysicsEntity_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(rand() % 360, rand() % 360, rand() % 360), glm::vec3(1.0f))
            .WithPBRMesh(modelName, "deferred_lit", 0.1f, 0.6f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, bodyEntity);
        shape.type = st;
        shape.size = glm::vec3(1.0f);
        shape.radius = 0.5f;
        shape.height = 1.0f;
        shape.restitution = m_S4Restitution;
        shape.friction = m_S4Friction;

        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, bodyEntity);
        rb.mass = m_S4Mass;
        rb.isStatic = false;
        rb.linearDamping = m_S4LinearDamping;
        rb.angularDamping = m_S4AngularDamping;
    }

    auto& physicsSystem = GetSystem<PhysicsSystem>();
    physicsSystem.Update(scene, 0.0f);
    if (m_S4InitialImpulse > 0.0f)
    {
        auto view = scene.registry.view<RigidBodyComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name.rfind("PhysicsEntity_", 0) != 0)
                continue;
            auto& rb = view.get<RigidBodyComponent>(entity);
            if (rb.body)
            {
                float impulse = m_S4InitialImpulse * (0.75f + static_cast<float>(rand() % 100) / 100.0f);
                float angle = static_cast<float>(rand() % 628) * 0.01f;
                glm::vec3 direction = glm::normalize(glm::vec3(std::cos(angle), 0.18f, std::sin(angle)));
                rb.body->Activate(true);
                rb.body->ApplyCentralImpulse(direction * impulse);
            }
        }
    }
}
