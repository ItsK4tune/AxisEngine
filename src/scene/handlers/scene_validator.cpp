#include <scene/handlers/scene_validator.h>
#include <utils/logger.h>
#include <scene/scene.h>
#include <script/script_registry.h>
#include <app/application.h>
#include <ecs/component.h>
#include <physic/physic_world.h>
#include <utils/bullet_glm_helpers.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SceneHandlers
{
    void SceneValidator::ValidateParentChildRelationships(
        Scene &scene,
        const std::map<entt::entity, std::vector<std::string>> &deferredChildren)
    {
        if (deferredChildren.empty())
            return;

        auto view = scene.registry.view<InfoComponent>();
        for (const auto &[parentEntity, childNames] : deferredChildren)
        {
            for (const auto &childName : childNames)
            {
                entt::entity childEntity = entt::null;
                for (auto entity : view)
                {
                    if (view.get<InfoComponent>(entity).name == childName)
                    {
                        childEntity = entity;
                        break;
                    }
                }

                if (childEntity != entt::null)
                {
                    if (scene.registry.all_of<TransformComponent>(childEntity) &&
                        scene.registry.all_of<TransformComponent>(parentEntity))
                    {
                        auto &transform = scene.registry.get<TransformComponent>(childEntity);
                        transform.SetParent(childEntity, parentEntity, scene.registry, true);
                    }
                }
                else
                {
                    LOGGER_ERROR("SceneValidator") << "Child not found: " << childName
                              << " for Parent Entity ID: " << (uint32_t)parentEntity;
                }
            }
        }
    }

    void SceneValidator::ValidateLights(Scene &scene)
    {
        auto dirLightView = scene.registry.view<DirectionalLightComponent>();
        bool hasShadowCaster = false;
        entt::entity lastDirLight = entt::null;

        for (auto entity : dirLightView)
        {
            auto &light = dirLightView.get<DirectionalLightComponent>(entity);
            if (light.isCastShadow && light.active)
            {
                hasShadowCaster = true;
                break;
            }
            if (light.active)
                lastDirLight = entity;
        }
    }

    void SceneValidator::ValidateCamera(Scene &scene, Application *app)
    {
        if (scene.GetActiveCamera() != entt::null)
            return;

        auto renderableView = scene.registry.view<MeshRendererComponent>();
        bool hasRenderableEntities = false;
        for (auto entity : renderableView)
        {
            hasRenderableEntities = true;
            break;
        }

        if (!hasRenderableEntities)
            return;

        LOGGER_WARN("SceneValidator") << "No Active Camera found in scene! Creating Default Spectator Camera.";

        entt::entity camEntity = scene.createEntity();

        scene.registry.emplace<InfoComponent>(camEntity, "Default Spectator Camera", "Default");

        auto &trans = scene.registry.emplace<TransformComponent>(camEntity);
        trans.position = glm::vec3(0.0f, 2.0f, 10.0f);

        auto &cam = scene.registry.emplace<CameraComponent>(camEntity);
        cam.isPrimary = true;
        cam.fov = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 1000.0f;
        cam.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        std::string scriptName = "DefaultCameraController";
        Scriptable *scriptInstance = ScriptRegistry::Instance().Create(scriptName);

        if (scriptInstance)
        {
            auto &scriptComp = scene.registry.emplace<ScriptComponent>(camEntity);
            scriptComp.instance = scriptInstance;
            scriptComp.InstantiateScript = [scriptName]()
            { return ScriptRegistry::Instance().Create(scriptName); };
            scriptComp.DestroyScript = [](ScriptComponent *nsc)
            { delete nsc->instance; nsc->instance = nullptr; };

            scriptComp.instance->Init(camEntity, &scene, app);
            scriptComp.instance->OnCreate();
            LOGGER_INFO("SceneValidator") << "Attached 'DefaultCameraController' (Engine Fallback) to default camera.";
        }
        else
        {
            LOGGER_WARN("SceneValidator") << "'DefaultCameraController' script not found! Make sure it is compiled.";
        }
    }

    void SceneValidator::ValidatePhysicsSync(Scene &scene, PhysicsWorld &phys)
    {
        auto rbView = scene.registry.view<RigidBodyComponent, TransformComponent>();
        for (auto entity : rbView)
        {
            auto &rb = rbView.get<RigidBodyComponent>(entity);
            auto &transform = rbView.get<TransformComponent>(entity);

            if (rb.body)
            {
                glm::mat4 worldMatrix = transform.GetWorldModelMatrix(scene.registry);
                glm::vec3 position = glm::vec3(worldMatrix[3]);
                glm::quat rotation = glm::quat_cast(worldMatrix);

                btTransform tr;
                tr.setIdentity();
                tr.setOrigin(BulletGLMHelpers::convert(position));
                tr.setRotation(BulletGLMHelpers::convert(rotation));

                rb.body->setWorldTransform(tr);
                if (rb.body->getMotionState())
                {
                    rb.body->getMotionState()->setWorldTransform(tr);
                }

                rb.body->setLinearVelocity(btVector3(0, 0, 0));
                rb.body->setAngularVelocity(btVector3(0, 0, 0));
                rb.body->activate();

                if (rb.isAttachedToParent && scene.registry.valid(transform.parent))
                {
                    if (scene.registry.all_of<RigidBodyComponent>(transform.parent))
                    {
                        auto &parentRb = scene.registry.get<RigidBodyComponent>(transform.parent);
                        if (parentRb.body)
                        {
                            btTransform frameInA, frameInB;

                            btTransform parentWorldTrans = parentRb.body->getWorldTransform();
                            btTransform childWorldTrans = rb.body->getWorldTransform();

                            frameInA = parentWorldTrans.inverse() * childWorldTrans;
                            frameInB.setIdentity();

                            btFixedConstraint *fixedConstraint = new btFixedConstraint(
                                *parentRb.body,
                                *rb.body,
                                frameInA,
                                frameInB);

                            phys.AddConstraint(fixedConstraint);
                            rb.constraint = fixedConstraint;
                        }
                    }
                }
            }
        }
    }
}
