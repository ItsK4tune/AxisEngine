#include "sample_scenario_common.h"

void SampleState::LoadScene21()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(m_S21Gravity);
        physics->SetSolverIterations(24);
    }
    if (auto* collisionMatrix = Resolve<CollisionMatrix>())
    {
        collisionMatrix->IgnoreTagCollision("chain_link", "chain_link");
        collisionMatrix->IgnoreTagCollision("chain_anchor", "chain_link");
        collisionMatrix->IgnoreTagCollision("chain_link", "chain_payload");
    }

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.2f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("FloorPhysics")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .WithRigidShape(ShapeType::Box, glm::vec3(80.0f, 0.5f, 80.0f), 1.0f, 2.0f, 0.5f, 0.5f)
        .WithRigidBody(0.0f, true)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    // 2. Hanging anchor (Static body, high up)
    glm::vec3 anchorPos(0.0f, 22.0f, 0.0f);
    anchorPos.y = m_S21AnchorHeight;
    auto anchor = EntityBuilder(scene, res, "scenario")
                      .WithName("Anchor")
                      .WithTag("chain_anchor")
                      .WithTransform(anchorPos, glm::vec3(0.0f), glm::vec3(1.5f))
                      .WithPBRMesh("cubeModel", "deferred_lit", 0.8f, 0.2f, 1.0f)
                      .WithRigidShape(ShapeType::Box, glm::vec3(1.0f), 1.0f, 2.0f, 0.5f, 0.5f)
                      .WithRigidBody(0.0f, true, false, 0.2f, 0.8f)
                      .Build();

    // 3. Dynamically build chain components
    m_S21ChainEntities.clear();
    m_S21ChainEntities.push_back(anchor);

    const Scenario21ShapeSpec linkSpec = GetScenario21ShapeSpec(m_S21LinkShape, false);
    const float linkOffset = linkSpec.shape == ShapeType::Capsule ? 1.75f : 1.45f;
    for (int i = 0; i < m_S21ChainLength; ++i)
    {
        glm::vec3 currentPos = anchorPos - glm::vec3(0.0f, linkOffset * (i + 1), 0.0f);

        auto link = EntityBuilder(scene, res, "scenario")
                        .WithName("ChainLink_" + std::to_string(i))
                        .WithTag("chain_link")
                        .WithTransform(currentPos, glm::vec3(0.0f), linkSpec.visualScale)
                        .WithPBRMesh(linkSpec.mesh, "deferred_lit", 0.1f, 0.5f, 1.0f)
                        .WithRigidShape(linkSpec.shape, linkSpec.boxSize, linkSpec.radius, linkSpec.height, 0.65f, 0.02f)
                        .WithRigidBody(m_S21LinkMass, false, false, m_S21LinkDamping, 0.9f)
                        .Build();

        m_S21ChainEntities.push_back(link);
    }

    auto payloadPos = anchorPos - glm::vec3(0.0f, linkOffset * m_S21ChainLength + 1.55f, 0.0f);
    const Scenario21ShapeSpec payloadSpec = GetScenario21ShapeSpec(m_S21PayloadShape, true);
    auto payload = EntityBuilder(scene, res, "scenario")
                       .WithName("Payload")
                       .WithTag("chain_payload")
                       .WithTransform(payloadPos, glm::vec3(0.0f), payloadSpec.visualScale)
                       .WithPBRMesh(payloadSpec.mesh, "deferred_lit", 0.05f, 0.45f, 1.0f)
                       .WithRigidShape(payloadSpec.shape, payloadSpec.boxSize, payloadSpec.radius, payloadSpec.height, 0.65f, 0.2f)
                       .WithRigidBody(m_S21PayloadMass, false, false, 0.2f, 0.85f)
                       .Build();
    m_S21ChainEntities.push_back(payload);

    for (int i = 0; i < 8; ++i)
    {
        const bool sphere = (i % 2) == 0;
        const float x = -10.5f + static_cast<float>(i) * 3.0f;
        const float z = (i % 3 == 0) ? -3.0f : 3.0f;
        const float y = anchorPos.y + 4.0f + static_cast<float>(i % 4) * 1.4f;
        const glm::vec3 scale = sphere ? glm::vec3(0.75f) : glm::vec3(0.85f);

        EntityBuilder(scene, res, "scenario")
            .WithName("GravityProbe_" + std::to_string(i))
            .WithTag("gravity_probe")
            .WithTransform(glm::vec3(x, y, z), glm::vec3(rand() % 360, rand() % 360, rand() % 360), scale)
            .WithPBRMesh(sphere ? "sphereModel" : "cubeModel", "deferred_lit", 0.0f, 0.38f, 1.0f)
            .WithRigidShape(sphere ? ShapeType::Sphere : ShapeType::Box, glm::vec3(1.0f), 1.0f, 2.0f, 0.6f, 0.35f)
            .WithRigidBody(0.75f + static_cast<float>(i) * 0.25f, false, false, 0.02f, 0.08f)
            .Build();
    }

    // Force PhysicsSystem to initialize bullet body structures immediately
    auto& physicsSys = GetSystem<PhysicsSystem>();
    physicsSys.Update(scene, 0.0f);

    // 4. Create constraints between consecutive chain links
    auto physics_ptr = Resolve<IPhysicsWorld>();
    if (physics_ptr)
    {
        for (size_t i = 1; i + 1 < m_S21ChainEntities.size(); ++i)
        {
            entt::entity prevEntity = m_S21ChainEntities[i - 1];
            entt::entity link = m_S21ChainEntities[i];

            auto& prevRBComp = scene.GetComponent<RigidBodyComponent>(prevEntity);
            auto& linkRBComp = scene.GetComponent<RigidBodyComponent>(link);

            if (prevRBComp.body && linkRBComp.body)
            {
                // Anchor is first element (index 0)
                glm::vec3 pivotA = (i == 1) ? glm::vec3(0.0f, -0.72f, 0.0f) : glm::vec3(0.0f, -0.48f, 0.0f);
                glm::vec3 pivotB = glm::vec3(0.0f, 0.48f, 0.0f);

                auto constraint =
                    physics_ptr->CreateHingeConstraint(prevRBComp.body, linkRBComp.body, pivotA, pivotB,
                                                       glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

                if (constraint)
                {
                    physics_ptr->AddConstraint(constraint);
                    prevRBComp.constraints.push_back(constraint);
                }
            }
        }

        if (m_S21ChainEntities.size() >= 2)
        {
            entt::entity prevEntity = m_S21ChainEntities[m_S21ChainEntities.size() - 2];
            entt::entity payloadEntity = m_S21ChainEntities.back();
            auto& prevRBComp = scene.GetComponent<RigidBodyComponent>(prevEntity);
            auto& payloadRBComp = scene.GetComponent<RigidBodyComponent>(payloadEntity);
            if (prevRBComp.body && payloadRBComp.body)
            {
                auto constraint = physics_ptr->CreatePoint2PointConstraint(
                    prevRBComp.body, payloadRBComp.body, glm::vec3(0.0f, -0.48f, 0.0f), glm::vec3(0.0f, 0.85f, 0.0f));
                if (constraint)
                {
                    physics_ptr->AddConstraint(constraint);
                    prevRBComp.constraints.push_back(constraint);
                }
            }
        }

        // Apply a starting side kick so it swings immediately
        auto lastLink = m_S21ChainEntities.back();
        auto* rbLast = scene.GetComponent<RigidBodyComponent>(lastLink).body.get();
        if (rbLast)
        {
            rbLast->Activate(true);
            rbLast->ApplyCentralImpulse(glm::vec3(m_S21KickForce, 0.0f, 0.0f));
        }
    }
}
