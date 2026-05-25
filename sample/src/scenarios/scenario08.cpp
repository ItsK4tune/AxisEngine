#include "sample_scenario_common.h"

void SampleState::LoadScene8()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(50.0f, 1.0f, 50.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();
    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floor);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(50.0f, 1.0f, 50.0f);
    floorShape.friction = 0.8f;
    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floor);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    // Controllable Player entity
    auto player = EntityBuilder(scene, res, "scenario")
        .WithName("PlayerCube")
        .WithTransform(glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(0.0f), glm::vec3(1.5f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.4f, 1.0f)
        .Build();

    std::string scriptName = "PlayerControlScript";
    auto& script = scene.registry.emplace<ScriptComponent>(player);
    script.className = scriptName;
    script.InstantiateScript = [scriptName]() {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(scriptName) : nullptr;
    };
    script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };

    auto& playerShape = EntityManager::AddComponent<RigidShapeComponent>(scene, player);
    playerShape.type = ShapeType::Box;
    playerShape.size = glm::vec3(1.5f);
    playerShape.friction = 0.7f;
    auto& playerRB = EntityManager::AddComponent<RigidBodyComponent>(scene, player);
    playerRB.mass = 1.0f;
    playerRB.isStatic = false;
    playerRB.isKinematic = true;
    playerRB.linearFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    playerRB.angularFactor = glm::vec3(0.0f, 1.0f, 0.0f);
    m_ShowDebugLines = false;
    ResetDefaultPlayerBindings();

    // Spawn some targets / visual obstacles to walk around
    for (int i = 0; i < 5; ++i)
    {
        float angle = static_cast<float>(i) * 72.0f * 3.14159f / 180.0f;
        float radius = 10.0f;
        glm::vec3 pos(cos(angle) * radius, 1.0f, sin(angle) * radius);

        auto target = EntityBuilder(scene, res, "scenario")
            .WithName("Target_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f), glm::vec3(2.0f))
            .WithMesh("sphereModel", "deferred_lit")
            .WithPBRMaterial(0.9f, 0.1f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, target);
        shape.type = ShapeType::Sphere;
        shape.radius = 1.0f;
        shape.friction = 0.5f;
        shape.restitution = 0.25f;
        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, target);
        rb.mass = 1.0f;
        rb.isStatic = false;
        rb.linearDamping = 0.25f;
        rb.angularDamping = 0.25f;
    }

    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    GetSystem<PhysicsSystem>().Update(scene, 0.0f);
}
