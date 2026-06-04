#include "sample_scenario_common.h"

void SampleState::LoadScene22()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    m_ShowDebugLines = true;
    m_S22HitCount = 0;
    m_S22LastHit = "No hit";
    m_S22RayOrigin = glm::vec3(0.0f, 5.0f, 24.0f);
    m_S22RayEnd = m_S22RayOrigin + glm::vec3(0.0f, 0.0f, -m_S22Distance);
    m_S22Targets.clear();
    ResetDefaultPlayerBindings();

    auto floor = EntityBuilder(scene, res, "scenario")
                     .WithName("S22QueryFloor")
                     .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
                     .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.75f, 1.0f)
                     .Build();
    {
        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, floor);
        ConfigureBoxCollider(shape, glm::vec3(1.0f, 0.05f, 1.0f));
        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, floor);
        rb.mass = 0.0f;
        rb.isStatic = true;
    }

    EntityBuilder(scene, res, "scenario")
        .WithName("S22DirLight")
        .WithTransform(glm::vec3(18.0f, 36.0f, 22.0f), glm::vec3(-45.0f, -35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.45f, -1.0f, -0.55f)), glm::vec3(1.0f), 1.35f)
        .Build();

    m_S22EmitterEntity = EntityBuilder(scene, res, "scenario")
                             .WithName("S22RayEmitter")
                             .WithTransform(m_S22RayOrigin, glm::vec3(0.0f), glm::vec3(1.2f))
                             .WithPBRMesh("cubeModel", "deferred_lit", 0.05f, 0.35f, 1.0f)
                             .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(m_S22EmitterEntity))
        renderer->color = glm::vec4(0.1f, 0.85f, 1.0f, 1.0f);

    struct TargetSpec
    {
        ShapeType shape;
        const char* model;
        glm::vec3 pos;
        glm::vec3 scale;
        glm::vec4 color;
    };

    const TargetSpec targets[] = {
        {ShapeType::Sphere, "sphereModel", glm::vec3(-14.0f, 1.8f, 2.0f), glm::vec3(1.7f),
         glm::vec4(0.35f, 0.80f, 0.30f, 1.0f)},
        {ShapeType::Box, "cubeModel", glm::vec3(-6.0f, 2.1f, -5.0f), glm::vec3(2.0f),
         glm::vec4(0.86f, 0.68f, 0.22f, 1.0f)},
        {ShapeType::Capsule, "capsuleModel", glm::vec3(6.0f, 1.7f, -10.0f), glm::vec3(1.35f),
         glm::vec4(0.65f, 0.46f, 1.0f, 1.0f)},
        {ShapeType::Sphere, "sphereModel", glm::vec3(14.0f, 1.8f, -16.0f), glm::vec3(1.7f),
         glm::vec4(0.28f, 0.76f, 0.95f, 1.0f)},
        {ShapeType::Box, "cubeModel", glm::vec3(-11.0f, 2.1f, -24.0f), glm::vec3(2.0f),
         glm::vec4(0.95f, 0.42f, 0.28f, 1.0f)},
        {ShapeType::Capsule, "capsuleModel", glm::vec3(10.0f, 1.7f, -31.0f), glm::vec3(1.35f),
         glm::vec4(0.92f, 0.36f, 0.74f, 1.0f)},
    };

    for (int i = 0; i < static_cast<int>(sizeof(targets) / sizeof(targets[0])); ++i)
    {
        const auto& spec = targets[i];
        auto target = EntityBuilder(scene, res, "scenario")
                          .WithName("S22Target_" + std::to_string(i))
                          .WithTransform(spec.pos, glm::vec3(0.0f, i * 19.0f, 0.0f), spec.scale)
                          .WithPBRMesh(spec.model, "deferred_lit", 0.04f, 0.42f, 1.0f)
                          .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(target))
            renderer->color = spec.color;

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, target);
        ConfigurePrimitiveCollider(shape, spec.shape);
        shape.restitution = 0.4f;
        shape.friction = 0.55f;

        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, target);
        rb.mass = 1.0f;
        rb.isStatic = false;
        rb.linearDamping = 0.35f;
        rb.angularDamping = 0.55f;

        m_S22Targets.push_back(target);
    }

    auto& physicsSystem = GetSystem<PhysicsSystem>();
    physicsSystem.Update(scene, 0.0f);
}
