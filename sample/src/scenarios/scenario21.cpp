#include "sample_scenario_common.h"
#include <physics/type/shape_type.h>

namespace
{
struct Scenario21ShapeSpec
{
    const char* mesh;
    ShapeType shape;
    glm::vec3 visualScale;
    glm::vec3 boxSize;
    float radius;
    float height;
};

Scenario21ShapeSpec GetScenario21ShapeSpec(int shapeIndex, bool payload)
{
    switch (shapeIndex)
    {
        case 1:
            return {"sphereModel", ShapeType::Sphere,    payload ? glm::vec3(1.65f) : glm::vec3(0.85f), glm::vec3(1.0f),
                    0.5f,          payload ? 1.0f : 0.8f};
        case 2:
            return {"capsuleModel",
                    ShapeType::Capsule,
                    payload ? glm::vec3(1.35f, 1.9f, 1.35f) : glm::vec3(0.65f, 1.15f, 0.65f),
                    glm::vec3(1.0f),
                    0.5f,
                    payload ? 1.05f : 0.85f};
        default:
            return {"cubeModel",     ShapeType::Box,        payload ? glm::vec3(1.6f) : glm::vec3(0.75f, 0.95f, 0.75f),
                    glm::vec3(0.5f), payload ? 1.0f : 0.5f, payload ? 1.0f : 0.8f};
    }
}
} // namespace

void SampleState::LoadScene21()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    SetPhysicsGravity(m_S21Gravity);
    SetPhysicsSolverIterations(24);
    IgnoreTagCollision("chain_link", "chain_link");
    IgnoreTagCollision("chain_anchor", "chain_link");
    IgnoreTagCollision("chain_link", "chain_payload");

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
    ForcePhysicsUpdate(0.0f);

    // 4. Create constraints between consecutive chain links
    for (size_t i = 1; i + 1 < m_S21ChainEntities.size(); ++i)
    {
        Entity prevEntity(m_S21ChainEntities[i - 1], &scene);
        Entity link(m_S21ChainEntities[i], &scene);

        // Anchor is first element (index 0)
        glm::vec3 pivotA = (i == 1) ? glm::vec3(0.0f, -0.72f, 0.0f) : glm::vec3(0.0f, -0.48f, 0.0f);
        glm::vec3 pivotB = glm::vec3(0.0f, 0.48f, 0.0f);

        CreateHingeConstraint(prevEntity, link, pivotA, pivotB, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    if (m_S21ChainEntities.size() >= 2)
    {
        Entity prevEntity(m_S21ChainEntities[m_S21ChainEntities.size() - 2], &scene);
        Entity payloadEntity(m_S21ChainEntities.back(), &scene);
        CreatePointToPointConstraint(prevEntity, payloadEntity, glm::vec3(0.0f, -0.48f, 0.0f), glm::vec3(0.0f, 0.85f, 0.0f));
    }

    // Apply a starting side kick so it swings immediately
    Entity lastLink(m_S21ChainEntities.back(), &scene);
    lastLink.ApplyCentralImpulse(glm::vec3(m_S21KickForce, 0.0f, 0.0f));
}
