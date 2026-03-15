#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <core/logic/app_framework.h>
#include <ecs/manager/entity_manager.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <physics/strategy/bullet/bullet_physics_world.h>
#include <physics/strategy/bullet/bullet_rigid_body.h>
#include <scene/logic/scene_validator.h>
#include <scene/logic/scene.h>
#include <script/logic/script_registry.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <core/logic/logger.h>

namespace SceneHandlers
{
    void SceneValidator::ValidateParentChildRelationships(
        Scene &scene,
        const std::map<entt::entity, std::vector<std::string>> &deferredChildren)
    {
        if (deferredChildren.empty())
            return;

        auto view = scene.registry.view<InfoComponent>();
        for (const auto &[childEntity, parentNames] : deferredChildren)
        {
            for (const auto &parentName : parentNames)
            {
                entt::entity parentEntity = entt::null;
                for (auto entity : view)
                {
                    if (view.get<InfoComponent>(entity).name == parentName)
                    {
                        parentEntity = entity;
                        break;
                    }
                }

                if (parentEntity != entt::null)
                {
                    if (scene.registry.all_of<HierarchyComponent>(childEntity) &&
                        scene.registry.all_of<HierarchyComponent>(parentEntity))
                    {
                        EntityManager::AddChild(scene, parentEntity, childEntity, true);
                    }
                }
                else
                {
                    LOGGER_ERROR("SceneValidator") << "Parent not found: " << parentName
                              << " for Child Entity ID: " << (uint32_t)childEntity;
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

    void SceneValidator::ValidateCamera(Scene &scene, EngineContext ctx)
    {
        if (EntityManager::GetActiveCamera(scene) != entt::null)
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

        entt::entity camEntity = EntityManager::CreateEntity(scene);

        auto &info = scene.registry.get<InfoComponent>(camEntity);
        info.name = "Default Spectator Camera";
        info.tag = "Default";

        auto &pos = scene.registry.get<PositionComponent>(camEntity);
        pos.value = glm::vec3(0.0f, 2.0f, 10.0f);
        pos.prev = pos.value;

        auto &cam = scene.registry.emplace<CameraComponent>(camEntity);
        cam.isPrimary = true;
        cam.fov = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 1000.0f;
        cam.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        cam.aspectRatio = (float)ctx.io->GetMonitorManager().GetWidth() / (float)ctx.io->GetMonitorManager().GetHeight();

        std::string scriptName = "DefaultCameraController";
        auto scriptInstance = ScriptRegistry::Instance().Create(scriptName);

        if (scriptInstance)
        {
            auto &scriptComp = scene.registry.emplace<ScriptComponent>(camEntity);
            scriptComp.instance = std::move(scriptInstance);
            scriptComp.InstantiateScript = [scriptName]()
            { return ScriptRegistry::Instance().Create(scriptName); };
            scriptComp.DestroyScript = [](ScriptComponent *nsc)
            { nsc->instance.reset(); };

            scriptComp.instance->Initialize(camEntity, &scene, ctx);
            scriptComp.instance->OnCreate();
            LOGGER_INFO("SceneValidator") << "Attached 'DefaultCameraController' (Engine Fallback) to default camera.";
        }
        else
        {
            LOGGER_WARN("SceneValidator") << "'DefaultCameraController' script not found! Make sure it is compiled.";
        }
    }

    void SceneValidator::ValidatePhysicsSync(Scene &scene, IPhysicsWorld &phys)
    {
        BulletPhysicsWorld* bulletWorld = dynamic_cast<BulletPhysicsWorld*>(&phys);
        if (!bulletWorld) {
             LOGGER_ERROR("SceneValidator") << "Physics world is not BulletPhysicsWorld, ignoring validation.";
             return;
        }

        auto rbView = scene.registry.view<RigidBodyComponent, WorldTransformComponent>();
        for (auto entity : rbView)
        {
            auto &rb = rbView.get<RigidBodyComponent>(entity);
            auto &world = rbView.get<WorldTransformComponent>(entity);

            if (rb.body)
            {
                BulletRigidBody* bulletBody = dynamic_cast<BulletRigidBody*>(rb.body.get());
                if (!bulletBody) continue;

                glm::mat4 worldMatrix = world.worldMatrix;
                glm::vec3 position = glm::vec3(worldMatrix[3]);
                glm::quat rotation = glm::quat_cast(worldMatrix);

                btTransform tr;
                tr.setIdentity();
                tr.setOrigin(BulletGLMHelpers::convert(position));
                tr.setRotation(BulletGLMHelpers::convert(rotation));

                bulletBody->GetRaw()->setWorldTransform(tr);
                if (bulletBody->GetRaw()->getMotionState())
                {
                    bulletBody->GetRaw()->getMotionState()->setWorldTransform(tr);
                }

                bulletBody->GetRaw()->setLinearVelocity(btVector3(0, 0, 0));
                bulletBody->GetRaw()->setAngularVelocity(btVector3(0, 0, 0));
                bulletBody->GetRaw()->activate();
                bulletBody->SetUserPointer((void*)(uintptr_t)entity);

                if (rb.isAttachedToParent && scene.registry.all_of<HierarchyComponent>(entity))
                {
                    auto &hier = scene.registry.get<HierarchyComponent>(entity);
                    if (scene.registry.valid(hier.parent) && scene.registry.all_of<RigidBodyComponent>(hier.parent))
                    {
                        auto &parentRb = scene.registry.get<RigidBodyComponent>(hier.parent);
                        if (parentRb.body)
                        {
                            BulletRigidBody* parentBulletBody = dynamic_cast<BulletRigidBody*>(parentRb.body.get());
                            if (parentBulletBody)
                            {
                                btTransform frameInA, frameInB;

                                btTransform parentWorldTrans = parentBulletBody->GetRaw()->getWorldTransform();
                                btTransform childWorldTrans = bulletBody->GetRaw()->getWorldTransform();

                                frameInA = parentWorldTrans.inverse() * childWorldTrans;
                                frameInB.setIdentity();

                                btFixedConstraint *fixedConstraint = new btFixedConstraint(
                                    *parentBulletBody->GetRaw(),
                                    *bulletBody->GetRaw(),
                                    frameInA,
                                    frameInB);

                                bulletWorld->GetRawWorld()->addConstraint(fixedConstraint);

                            }
                        }
                    }
                }
            }
        }
    }
}





