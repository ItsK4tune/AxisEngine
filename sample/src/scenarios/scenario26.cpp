#include "sample_scenario_common.h"

void SampleState::LoadScene26()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    auto* phys = Resolve<IPhysicsWorld>();

    ResetDefaultPlayerBindings();
    Scenario26CharacterControllerScript::s_JumpCount = 0;
    Scenario26FpsCameraScript::s_Yaw = -90.0f;
    Scenario26FpsCameraScript::s_Pitch = -8.0f;

    entt::entity camera = entt::null;
    auto cameraView = scene.View<InfoComponent>();
    for (auto entity : cameraView)
    {
        if (cameraView.get<InfoComponent>(entity).name == "MainCamera")
        {
            camera = entity;
            break;
        }
    }
    if (camera != entt::null && scene.IsValid(camera))
    {
        EntityBuilder(scene, res, camera)
            .WithPosition(glm::vec3(0.0f, 4.45f, 10.0f))
            .WithRotation(glm::quatLookAt(glm::normalize(glm::vec3(0.0f, -0.14f, -1.0f)), glm::vec3(0.0f, 1.0f, 0.0f)))
            .WithScriptable("Scenario26FpsCameraScript", []() { return std::make_unique<Scenario26FpsCameraScript>(); });
    }

    EntityBuilder(scene, res, "scenario")
        .WithName("S26_Light")
        .WithTransform(glm::vec3(18.0f, 35.0f, 16.0f), glm::vec3(-45.0f, -35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.55f, -1.0f, -0.4f)), glm::vec3(1.0f), 1.35f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("S26_Floor")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(70.0f, 1.0f, 70.0f))
        .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.8f, 1.0f)
        .WithRigidShape(ShapeType::Box, glm::vec3(1.0f, 0.5f, 1.0f))
        .WithRigidBody(0.0f, true)
        .Build();

    for (int i = 0; i < 5; ++i)
    {
        EntityBuilder(scene, res, "scenario")
            .WithName("S26_Step_" + std::to_string(i))
            .WithTransform(glm::vec3(-8.0f + i * 2.4f, 0.35f + i * 0.28f, -8.0f), glm::vec3(0.0f),
                           glm::vec3(2.2f, 0.35f + i * 0.28f, 4.0f))
            .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.65f, 1.0f)
            .WithRigidShape(ShapeType::Box)
            .WithRigidBody(0.0f, true)
            .Build();
    }

    struct StaticObstacleSpec
    {
        const char* name;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        glm::vec4 color;
    };

    const StaticObstacleSpec obstacles[] = {
        {"S26_Blocker_A", glm::vec3(8.0f, 1.1f, 3.0f), glm::vec3(0.0f), glm::vec3(2.4f, 1.1f, 5.0f),
         glm::vec4(0.75f, 0.32f, 0.26f, 1.0f)},
        {"S26_Blocker_B", glm::vec3(-12.0f, 0.9f, 7.0f), glm::vec3(0.0f), glm::vec3(5.0f, 0.9f, 1.8f),
         glm::vec4(0.32f, 0.54f, 0.82f, 1.0f)},
        {"S26_Ramp_A", glm::vec3(10.0f, 1.05f, -9.0f), glm::vec3(0.0f, 0.0f, -12.0f), glm::vec3(5.5f, 0.45f, 2.6f),
         glm::vec4(0.55f, 0.72f, 0.34f, 1.0f)},
        {"S26_Ramp_B", glm::vec3(-15.0f, 1.35f, -10.0f), glm::vec3(0.0f, 0.0f, 14.0f), glm::vec3(5.5f, 0.45f, 2.6f),
         glm::vec4(0.65f, 0.48f, 0.82f, 1.0f)},
        {"S26_NarrowGate_L", glm::vec3(-3.6f, 1.5f, -17.0f), glm::vec3(0.0f), glm::vec3(1.0f, 1.5f, 3.5f),
         glm::vec4(0.84f, 0.62f, 0.28f, 1.0f)},
        {"S26_NarrowGate_R", glm::vec3(3.6f, 1.5f, -17.0f), glm::vec3(0.0f), glm::vec3(1.0f, 1.5f, 3.5f),
         glm::vec4(0.84f, 0.62f, 0.28f, 1.0f)},
        {"S26_LowBar", glm::vec3(0.0f, 0.45f, 18.0f), glm::vec3(0.0f), glm::vec3(12.0f, 0.45f, 1.4f),
         glm::vec4(0.28f, 0.7f, 0.72f, 1.0f)},
    };

    for (const auto& spec : obstacles)
    {
        EntityBuilder(scene, res, "scenario")
            .WithName(spec.name)
            .WithTransform(spec.position, spec.rotation, spec.scale)
            .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.58f, 1.0f)
            .WithRigidShape(ShapeType::Box, glm::vec3(1.0f), 1.0f, 2.0f, 0.85f)
            .WithRigidBody(0.0f, true)
            .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(scene.FindByName(spec.name)))
            renderer->color = spec.color;
    }

    auto triggerZone = EntityBuilder(scene, res, "scenario")
                           .WithName("S26_TriggerZone")
                           .WithTag("trigger")
                           .WithTransform(glm::vec3(0.0f, 2.0f, 4.0f), glm::vec3(0.0f), glm::vec3(4.8f, 2.2f, 4.8f))
                           .WithPBRMesh("cubeModel", "forward_transparent", 0.0f, 0.3f, 1.0f)
                           .WithRigidShape(ShapeType::Box)
                           .WithRigidBody(0.0f, true, true)
                           .Build();
    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(triggerZone))
        renderer->color = glm::vec4(0.1f, 0.75f, 1.0f, 0.24f);
    if (auto* mat = scene.TryGetComponent<MaterialComponent>(triggerZone))
    {
        mat->desc.opacity = 0.24f;
        mat->gpu.dirty = true;
    }

    const auto createEffectZone = [&](const char* name, const glm::vec3& position, const glm::vec3& scale,
                                      const glm::vec4& color) {
        auto zone = EntityBuilder(scene, res, "scenario")
                        .WithName(name)
                        .WithTag("trigger")
                        .WithTransform(position, glm::vec3(0.0f), scale)
                        .WithPBRMesh("cubeModel", "forward_transparent", 0.0f, 0.28f, 1.0f)
                        .WithRigidShape(ShapeType::Box, glm::vec3(1.0f), 1.0f, 2.0f, 0.05f)
                        .WithRigidBody(0.0f, true, true)
                        .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(zone))
            renderer->color = color;
        if (auto* mat = scene.TryGetComponent<MaterialComponent>(zone))
        {
            mat->desc.opacity = color.a;
            mat->gpu.dirty = true;
        }
        return zone;
    };

    createEffectZone("S26_SlowZone", glm::vec3(-14.0f, 1.25f, 14.0f), glm::vec3(4.8f, 1.35f, 4.8f),
                     glm::vec4(0.2f, 0.28f, 1.0f, 0.22f));
    createEffectZone("S26_SinkZone", glm::vec3(-14.0f, 1.25f, -4.0f), glm::vec3(4.8f, 1.35f, 4.8f),
                     glm::vec4(0.18f, 0.12f, 0.05f, 0.24f));
    createEffectZone("S26_FlyZone", glm::vec3(14.0f, 2.6f, 14.0f), glm::vec3(4.8f, 3.0f, 4.8f),
                     glm::vec4(0.3f, 0.95f, 1.0f, 0.2f));
    createEffectZone("S26_SlipperyZone", glm::vec3(14.0f, 1.25f, -4.0f), glm::vec3(5.4f, 1.2f, 5.4f),
                     glm::vec4(0.72f, 0.95f, 1.0f, 0.22f));
    createEffectZone("S26_BoostZone", glm::vec3(0.0f, 1.25f, -18.0f), glm::vec3(5.4f, 1.35f, 4.4f),
                     glm::vec4(1.0f, 0.72f, 0.12f, 0.22f));

    auto callbackBlock =
        EntityBuilder(scene, res, "scenario")
            .WithName("S26_CallbackBlock")
            .WithTag("solid_callback")
            .WithTransform(glm::vec3(0.0f, 1.15f, -3.2f), glm::vec3(0.0f), glm::vec3(4.6f, 1.15f, 1.2f))
            .WithPBRMesh("cubeModel", "deferred_lit", 0.0f, 0.5f, 1.0f)
            .WithRigidShape(ShapeType::Box)
            .WithRigidBody(0.0f, true)
            .Build();
    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(callbackBlock))
        renderer->color = glm::vec4(0.95f, 0.55f, 0.18f, 1.0f);

    std::vector<entt::entity> fixedParts;
    fixedParts.reserve(2);
    for (int i = 0; i < 2; ++i)
    {
        auto part = EntityBuilder(scene, res, "scenario")
                        .WithName("S26_FixedConstraintPart_" + std::to_string(i))
                        .WithTransform(glm::vec3(12.5f + i * 2.2f, 3.4f, 5.5f), glm::vec3(0.0f), glm::vec3(1.0f))
                        .WithPBRMesh("cubeModel", "deferred_lit", 0.15f, 0.38f, 1.0f)
                        .WithRigidShape(ShapeType::Box)
                        .WithRigidBody(1.0f, false, false, 0.2f, 0.35f)
                        .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(part))
            renderer->color = glm::vec4(0.72f, 0.38f, 0.95f, 1.0f);
        fixedParts.push_back(part);
    }

    for (int i = 0; i < 8; ++i)
    {
        const bool sphere = (i % 2) == 0;
        const float x = -18.0f + static_cast<float>(i % 4) * 12.0f;
        const float z = -2.0f + static_cast<float>(i / 4) * 13.0f;
        auto target = EntityBuilder(scene, res, "scenario")
                          .WithName("S26_DraggableTarget_" + std::to_string(i))
                          .WithTransform(glm::vec3(x, 2.2f, z), glm::vec3(0.0f), glm::vec3(sphere ? 1.35f : 1.15f))
                          .WithPBRMesh(sphere ? "sphereModel" : "cubeModel", "deferred_lit", 0.08f, 0.42f, 1.0f)
                          .WithRigidShape(sphere ? ShapeType::Sphere : ShapeType::Box, glm::vec3(1.0f), 1.0f, 2.0f, 0.65f, 0.25f)
                          .WithRigidBody(1.0f, false, false, 0.25f, 0.45f)
                          .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(target))
            renderer->color = sphere ? glm::vec4(0.95f, 0.42f, 0.22f, 1.0f) : glm::vec4(0.28f, 0.78f, 0.95f, 1.0f);
    }

    const float initialMoveSpeed = m_S26MoveSpeed;
    const float initialSprintMultiplier = m_S26SprintMultiplier;
    const float initialSlowMultiplier = m_S26SlowMultiplier;
    const float initialJumpSpeed = m_S26JumpSpeed;
    const float initialStepHeight = m_S26StepHeight;
    const float initialMaxSlope = m_S26MaxSlope;
    const bool initialIgnoreCharacterTrigger = m_S26IgnoreCharacterTrigger;

    m_S26ControllerEntity = EntityBuilder(scene, res, "scenario")
                                .WithName("S26_CharacterController")
                                .WithTag("mover")
                                .WithTransform(glm::vec3(0.0f, 3.0f, 10.0f), glm::vec3(0.0f), glm::vec3(1.2f))
                                .WithPBRMesh("capsuleModel", "deferred_lit", 0.0f, 0.45f, 1.0f)
                                .WithScriptable("Scenario26CharacterControllerScript", [initialMoveSpeed, initialSprintMultiplier, initialSlowMultiplier,
                                                                                      initialJumpSpeed, initialStepHeight, initialMaxSlope,
                                                                                      initialIgnoreCharacterTrigger]() {
                                    auto script = std::make_unique<Scenario26CharacterControllerScript>();
                                    script->moveSpeed = initialMoveSpeed;
                                    script->sprintMultiplier = initialSprintMultiplier;
                                    script->slowMultiplier = initialSlowMultiplier;
                                    script->jumpSpeed = initialJumpSpeed;
                                    script->stepHeight = initialStepHeight;
                                    script->maxSlope = initialMaxSlope;
                                    script->ignoreCharacterTrigger = initialIgnoreCharacterTrigger;
                                    return script;
                                })
                                .Build();
    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(m_S26ControllerEntity))
        renderer->color = glm::vec4(0.2f, 0.95f, 0.35f, 1.0f);

    if (phys)
    {
        auto shape = phys->CreateCapsuleShape(0.82f, 1.2f);
        auto controller = phys->CreateCharacterController(shape, m_S26StepHeight);
        if (controller)
        {
            controller->SetWorldTransform(glm::vec3(0.0f, 3.0f, 10.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            controller->SetMaxSlope(glm::radians(m_S26MaxSlope));
            controller->SetJumpSpeed(m_S26JumpSpeed);
            controller->SetUserPointer((void*)((uintptr_t)m_S26ControllerEntity + 1));
            phys->AddCharacterController(controller.get());
            EntityBuilder(scene, res, m_S26ControllerEntity)
                .WithCharacterController(controller, m_S26StepHeight, m_S26MaxSlope);
        }
    }

    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    GetSystem<PhysicsSystem>().Update(scene, 0.0f);

    if (phys && fixedParts.size() == 2)
    {
        auto& rbA = scene.GetComponent<RigidBodyComponent>(fixedParts[0]);
        auto& rbB = scene.GetComponent<RigidBodyComponent>(fixedParts[1]);
        if (rbA.body && rbB.body)
        {
            auto constraint = phys->CreateFixedConstraint(
                rbA.body, rbB.body, glm::vec3(1.1f, 0.0f, 0.0f), glm::vec3(-1.1f, 0.0f, 0.0f),
                glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            if (constraint)
            {
                rbA.constraints.push_back(constraint);
                rbB.constraints.push_back(constraint);
                phys->AddConstraint(constraint);
            }
        }
    }
}
