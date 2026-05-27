#include "sample_scenario_common.h"

void SampleState::LoadScene10()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(m_S10Gravity);
        physics->SetSolverIterations(24);
    }
    if (auto* collisionMatrix = Resolve<CollisionMatrix>())
    {
        collisionMatrix->IgnoreTagCollision("chain_link", "chain_link");
    }

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithPBRMesh("planeModel", "deferred_lit", 0.2f, 0.8f, 1.0f)
        .Build();

    auto floorPhys = EntityBuilder(scene, res, "scenario")
        .WithName("FloorPhysics")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    
    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floorPhys);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(80.0f, 1.0f, 80.0f);
    floorShape.restitution = 0.5f;
    floorShape.friction = 0.5f;

    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floorPhys);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    // 2. Hanging anchor (Static body, high up)
    glm::vec3 anchorPos(0.0f, 22.0f, 0.0f);
    anchorPos.y = m_S10AnchorHeight;
    auto anchor = EntityBuilder(scene, res, "scenario")
        .WithName("Anchor")
        .WithTag("chain_anchor")
        .WithTransform(anchorPos, glm::vec3(0.0f), glm::vec3(1.5f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.8f, 0.2f, 1.0f)
        .Build();

    auto& anchorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, anchor);
    anchorShape.type = ShapeType::Box;
    anchorShape.size = glm::vec3(0.5f);
    anchorShape.restitution = 0.5f;
    anchorShape.friction = 0.5f;

    auto& anchorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, anchor);
    anchorRB.mass = 0.0f;
    anchorRB.isStatic = true;
    anchorRB.linearDamping = 0.2f;
    anchorRB.angularDamping = 0.8f;

    // 3. Dynamically build chain components
    m_S10ChainEntities.clear();
    m_S10ChainEntities.push_back(anchor);

    float linkOffset = 1.1f;
    const Scenario10ShapeSpec linkSpec = GetScenario10ShapeSpec(m_S10LinkShape, false);
    for (int i = 0; i < m_S10ChainLength; ++i)
    {
        glm::vec3 currentPos = anchorPos - glm::vec3(0.0f, linkOffset * (i + 1), 0.0f);
        
        auto link = EntityBuilder(scene, res, "scenario")
            .WithName("ChainLink_" + std::to_string(i))
            .WithTag("chain_link")
            .WithTransform(currentPos, glm::vec3(0.0f), linkSpec.visualScale)
            .WithPBRMesh(linkSpec.mesh, "deferred_lit", 0.1f, 0.5f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, link);
        ApplyShapeSpec(shape, linkSpec);
        shape.restitution = 0.15f;
        shape.friction = 0.65f;

        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, link);
        rb.mass = m_S10LinkMass;
        rb.isStatic = false;
        rb.linearDamping = m_S10LinkDamping;
        rb.angularDamping = 0.9f;

        m_S10ChainEntities.push_back(link);
    }

    auto payloadPos = anchorPos - glm::vec3(0.0f, linkOffset * m_S10ChainLength + 1.55f, 0.0f);
    const Scenario10ShapeSpec payloadSpec = GetScenario10ShapeSpec(m_S10PayloadShape, true);
    auto payload = EntityBuilder(scene, res, "scenario")
        .WithName("Payload")
        .WithTag("chain_payload")
        .WithTransform(payloadPos, glm::vec3(0.0f), payloadSpec.visualScale)
        .WithPBRMesh(payloadSpec.mesh, "deferred_lit", 0.05f, 0.45f, 1.0f)
        .Build();
    auto& payloadShape = EntityManager::AddComponent<RigidShapeComponent>(scene, payload);
    ApplyShapeSpec(payloadShape, payloadSpec);
    payloadShape.restitution = 0.2f;
    payloadShape.friction = 0.65f;
    auto& payloadRB = EntityManager::AddComponent<RigidBodyComponent>(scene, payload);
    payloadRB.mass = m_S10PayloadMass;
    payloadRB.isStatic = false;
    payloadRB.linearDamping = 0.2f;
    payloadRB.angularDamping = 0.85f;
    m_S10ChainEntities.push_back(payload);

    for (int i = 0; i < 8; ++i)
    {
        const bool sphere = (i % 2) == 0;
        const float x = -10.5f + static_cast<float>(i) * 3.0f;
        const float z = (i % 3 == 0) ? -3.0f : 3.0f;
        const float y = anchorPos.y + 4.0f + static_cast<float>(i % 4) * 1.4f;
        const glm::vec3 scale = sphere ? glm::vec3(0.75f) : glm::vec3(0.85f);

        auto probe = EntityBuilder(scene, res, "scenario")
                         .WithName("GravityProbe_" + std::to_string(i))
                         .WithTag("gravity_probe")
                         .WithTransform(glm::vec3(x, y, z),
                                        glm::vec3(rand() % 360, rand() % 360, rand() % 360), scale)
                         .WithPBRMesh(sphere ? "sphereModel" : "cubeModel", "deferred_lit", 0.0f, 0.38f, 1.0f)
                         .Build();

        auto& probeShape = EntityManager::AddComponent<RigidShapeComponent>(scene, probe);
        probeShape.type = sphere ? ShapeType::Sphere : ShapeType::Box;
        probeShape.size = glm::vec3(0.5f);
        probeShape.radius = 0.5f;
        probeShape.height = 1.0f;
        probeShape.restitution = 0.35f;
        probeShape.friction = 0.6f;

        auto& probeRB = EntityManager::AddComponent<RigidBodyComponent>(scene, probe);
        probeRB.mass = 0.75f + static_cast<float>(i) * 0.25f;
        probeRB.isStatic = false;
        probeRB.linearDamping = 0.02f;
        probeRB.angularDamping = 0.08f;
    }

    // Force PhysicsSystem to initialize bullet body structures immediately
    auto& physicsSys = GetSystem<PhysicsSystem>();
    physicsSys.Update(scene, 0.0f);

    // 4. Create constraints between consecutive chain links
    auto physics_ptr = Resolve<IPhysicsWorld>();
    if (physics_ptr)
    {
        for (size_t i = 1; i + 1 < m_S10ChainEntities.size(); ++i)
        {
            entt::entity prevEntity = m_S10ChainEntities[i - 1];
            entt::entity link = m_S10ChainEntities[i];

            auto& prevRBComp = scene.registry.get<RigidBodyComponent>(prevEntity);
            auto& linkRBComp = scene.registry.get<RigidBodyComponent>(link);

            if (prevRBComp.body && linkRBComp.body)
            {
                // Anchor is first element (index 0)
                glm::vec3 pivotA = (i == 1) ? glm::vec3(0.0f, -0.75f, 0.0f) : glm::vec3(0.0f, -0.52f, 0.0f);
                glm::vec3 pivotB = glm::vec3(0.0f, 0.52f, 0.0f);

                auto constraint = physics_ptr->CreateHingeConstraint(
                    prevRBComp.body,
                    linkRBComp.body,
                    pivotA, pivotB,
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
                );

                if (constraint)
                {
                    physics_ptr->AddConstraint(constraint);
                    prevRBComp.constraints.push_back(constraint);
                }
            }
        }

        if (m_S10ChainEntities.size() >= 2)
        {
            entt::entity prevEntity = m_S10ChainEntities[m_S10ChainEntities.size() - 2];
            entt::entity payloadEntity = m_S10ChainEntities.back();
            auto& prevRBComp = scene.registry.get<RigidBodyComponent>(prevEntity);
            auto& payloadRBComp = scene.registry.get<RigidBodyComponent>(payloadEntity);
            if (prevRBComp.body && payloadRBComp.body)
            {
                auto constraint = physics_ptr->CreatePoint2PointConstraint(prevRBComp.body, payloadRBComp.body,
                                                                          glm::vec3(0.0f, -0.52f, 0.0f),
                                                                          glm::vec3(0.0f, 1.0f, 0.0f));
                if (constraint)
                {
                    physics_ptr->AddConstraint(constraint);
                    prevRBComp.constraints.push_back(constraint);
                }
            }
        }

        // Apply a starting side kick so it swings immediately
        auto lastLink = m_S10ChainEntities.back();
        auto* rbLast = scene.registry.get<RigidBodyComponent>(lastLink).body.get();
        if (rbLast)
        {
            rbLast->Activate(true);
            rbLast->ApplyCentralImpulse(glm::vec3(m_S10KickForce, 0.0f, 0.0f));
        }
    }
}
