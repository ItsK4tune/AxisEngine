#include "sample_scenario_common.h"

void SampleState::LoadScene20()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    SetPhysicsGravity(m_S20Gravity);

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .WithRigidShape(ShapeType::Box, glm::vec3(1.0f, 0.5f, 1.0f), 1.0f, 2.0f, m_S20Friction, m_S20Restitution)
        .WithRigidBody(0.0f, true)
        .Build();

    std::string modelName = "cubeModel";
    ShapeType st = ShapeType::Box;
    if (m_S20ShapeType == 1)
    {
        modelName = "sphereModel";
        st = ShapeType::Sphere;
    }
    else if (m_S20ShapeType == 2)
    {
        modelName = "capsuleModel";
        st = ShapeType::Capsule;
    }

    for (int i = 0; i < m_S20EntityCount; ++i)
    {
        float x =
            static_cast<float>((i % 10) - 5) * m_S20GridSpacing + (static_cast<float>(rand() % 100) / 400.0f - 0.125f);
        float y = static_cast<float>(i / 25) * m_S20GridSpacing + m_S20SpawnHeight;
        float z = static_cast<float>(((i / 10) % 10) - 5) * m_S20GridSpacing +
                  (static_cast<float>(rand() % 100) / 400.0f - 0.125f);

        EntityBuilder(scene, res, "scenario")
            .WithName("PhysicsEntity_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(rand() % 360, rand() % 360, rand() % 360), glm::vec3(1.0f))
            .WithPBRMesh(modelName, "deferred_lit", 0.1f, 0.6f, 1.0f)
            .WithRigidShape(st, glm::vec3(1.0f), 1.0f, 2.0f, m_S20Friction, m_S20Restitution)
            .WithRigidBody(m_S20Mass, false, false, m_S20LinearDamping, m_S20AngularDamping)
            .Build();
    }

    ForcePhysicsUpdate(0.0f);
    if (m_S20InitialImpulse > 0.0f)
    for (auto entity : GetEntitiesWithNamePrefix("PhysicsEntity_"))
    {
        float impulse = m_S20InitialImpulse * (0.75f + static_cast<float>(rand() % 100) / 100.0f);
        float angle = static_cast<float>(rand() % 628) * 0.01f;
        glm::vec3 direction = glm::normalize(glm::vec3(std::cos(angle), 0.18f, std::sin(angle)));
        entity.ApplyCentralImpulse(direction * impulse);
    }
}
