#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <interface/physics/i_rigid_body.h>

struct RigidBodyComponent
{
    std::shared_ptr<IRigidBody> body = nullptr;

    bool isAttachedToParent = false;

    bool isParentMatter = false;
    bool isChildrenMatter = false;

    glm::vec3 linearFactor = glm::vec3(1.0f);
    glm::vec3 angularFactor = glm::vec3(1.0f);

    void SetRestitution(float restitution)
    {
        if (body)
            body->SetRestitution(restitution);
    }

    void SetFriction(float friction)
    {
        if (body)
            body->SetFriction(friction);
    }

    void SetLinearFactor(const glm::vec3 &factor)
    {
        if (body)
            body->SetLinearFactor(factor);
    }

    void SetAngularFactor(const glm::vec3 &factor)
    {
        if (body)
            body->SetAngularFactor(factor);
    }

    void SetLinearVelocity(const glm::vec3 &vel)
    {
        if (body)
        {
            body->SetLinearVelocity(vel);
            body->Activate(true);
        }
    }

    void SetAngularVelocity(const glm::vec3 &vel)
    {
        if (body)
        {
            body->SetAngularVelocity(vel);
            body->Activate(true);
        }
    }
};
