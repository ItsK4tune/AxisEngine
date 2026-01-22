#include <physic/physics_loader.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#include <iostream>

void PhysicsLoader::LoadRigidBody(Scene &scene, entt::entity entity, std::stringstream &ss, PhysicsWorld &physics, std::ifstream &file)
{
    std::string type;
    float mass;
    ss >> type >> mass;

    auto &trans = scene.registry.get<TransformComponent>(entity);
    auto &rb = scene.registry.emplace<RigidBodyComponent>(entity);

    btCollisionShape *finalShape = nullptr;
    if (type == "COMPOUND")
    {
        btCompoundShape *compound = new btCompoundShape();
        std::string subLine;

        while (std::getline(file, subLine))
        {
            std::stringstream subSS(subLine);
            std::string subCmd;
            subSS >> subCmd;

            if (subCmd == "END_RIGIDBODY")
                break;

            if (subCmd == "SHAPE")
            {
                std::string shapeType;
                float lx, ly, lz, lrx, lry, lrz;
                subSS >> shapeType >> lx >> ly >> lz >> lrx >> lry >> lrz;

                btTransform localTrans;
                localTrans.setIdentity();
                localTrans.setOrigin(btVector3(lx, ly, lz));
                btQuaternion localRot;
                localRot.setEuler(glm::radians(lry), glm::radians(lrx), glm::radians(lrz));
                localTrans.setRotation(localRot);

                btCollisionShape *childShape = nullptr;

                if (shapeType == "BOX")
                {
                    float x, y, z;
                    subSS >> x >> y >> z;
                    childShape = new btBoxShape(btVector3(x, y, z));
                }
                else if (shapeType == "SPHERE")
                {
                    float r;
                    subSS >> r;
                    childShape = new btSphereShape(r);
                }
                else if (shapeType == "CAPSULE")
                {
                    float r, h;
                    subSS >> r >> h;
                    childShape = new btCapsuleShape(r, h);
                }

                if (childShape)
                {
                    compound->addChildShape(localTrans, childShape);
                    physics.RegisterShape(childShape);
                }
            }
        }
        finalShape = compound;
    }
    else if (type == "CAPSULE")
    {
        float r, h;
        ss >> r >> h;
        finalShape = new btCapsuleShape(r, h);
    }
    else if (type == "BOX")
    {
        float x, y, z;
        ss >> x >> y >> z;
        finalShape = new btBoxShape(btVector3(x, y, z));
    }

    glm::vec3 centerOffset(0.0f);
    glm::vec3 rotFactor(1.0f);
    glm::vec3 posFactor(1.0f);
    bool hasRotFactor = false;
    bool hasPosFactor = false;
    float restitution = 0.0f;
    std::string bodyType = "UNKNOWN";
    std::string nextToken;

    while (ss >> nextToken)
    {
        if (nextToken == "OFFSET")
        {
            float ox, oy, oz;
            ss >> ox >> oy >> oz;
            centerOffset = glm::vec3(ox, oy, oz);
        }
        else if (nextToken == "RESTITUTION")
        {
            ss >> restitution;
        }
        else if (nextToken == "ROT_FACTOR" || nextToken == "LOCK_ANGULAR")
        {
            float x, y, z;
            ss >> x >> y >> z;
            rb.angularFactor = glm::vec3(x, y, z);
            hasRotFactor = true;
        }
        else if (nextToken == "POS_FACTOR" || nextToken == "LOCK_LINEAR")
        {
            float x, y, z;
            ss >> x >> y >> z;
            rb.linearFactor = glm::vec3(x, y, z);
            hasPosFactor = true;
        }
        else if (nextToken == "PARENT_MATTER" || nextToken == "IS_PARENT_MATTER")
        {
            rb.isParentMatter = true;
        }
        else if (nextToken == "CHILDREN_MATTER" || nextToken == "IS_CHILDREN_MATTER")
        {
            rb.isChildrenMatter = true;
        }
        else if (nextToken == "STATIC")
        {
            bodyType = "STATIC";
        }
        else if (nextToken == "DYNAMIC")
        {
            bodyType = "DYNAMIC";
        }
        else if (nextToken == "KINEMATIC")
        {
            bodyType = "KINEMATIC";
        }
        else if (nextToken == "ATTACH_TO_PARENT")
        {
            rb.isAttachedToParent = true;
        }
    }

    if (finalShape && glm::length(centerOffset) > 0.001f)
    {
        btCompoundShape *compound = new btCompoundShape();
        btTransform localTrans;
        localTrans.setIdentity();
        localTrans.setOrigin(BulletGLMHelpers::convert(centerOffset));
        compound->addChildShape(localTrans, finalShape);
        finalShape = compound;
        physics.RegisterShape(finalShape);
    }

    if (finalShape)
    {
        if (bodyType == "UNKNOWN")
        {
            if (mass > 0.0f)
                bodyType = "DYNAMIC";
            else
                bodyType = "STATIC";
        }

        if (bodyType == "STATIC")
            mass = 0.0f;
        if (bodyType == "KINEMATIC")
            mass = 0.0f;

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(BulletGLMHelpers::convert(trans.position));
        transform.setRotation(BulletGLMHelpers::convert(trans.rotation));

        rb.body = physics.CreateRigidBody(mass, transform, finalShape);

        if (rb.body)
        {
            if (bodyType == "KINEMATIC")
            {
                rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                rb.body->setActivationState(DISABLE_DEACTIVATION);
            }

            rb.body->setUserPointer((void *)(uintptr_t)entity);

            if (type == "CAPSULE" || type == "PLAYER")
            {
                 // Default capsule lock rotation, but allow override
                 if (!hasRotFactor)
                    rb.angularFactor = glm::vec3(0, 1, 0);
            }

            rb.body->setAngularFactor(BulletGLMHelpers::convert(rb.angularFactor));
            rb.body->setLinearFactor(BulletGLMHelpers::convert(rb.linearFactor));

            if (restitution > 0.0f)
            {
                rb.body->setRestitution(restitution);
            }
        }
    }
}
