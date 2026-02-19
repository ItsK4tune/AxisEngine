#pragma once

#include <interface/physics/i_rigid_body.h>
#include <btBulletDynamicsCommon.h>

class BulletRigidBody : public IRigidBody
{
public:
    BulletRigidBody(btRigidBody* body, std::shared_ptr<ICollisionShape> shape) 
        : m_Body(body), m_Shape(shape) {}
    ~BulletRigidBody()
    {
        if (m_Body)
        {
            if (m_Body->getMotionState()) delete m_Body->getMotionState();
            delete m_Body;
        }
    }

    // ... (rest of methods)





    void SetLinearVelocity(const glm::vec3& vel) override
    {
        if (m_Body)
        {
            m_Body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
            m_Body->activate(true);
        }
    }

    void SetAngularVelocity(const glm::vec3& vel) override
    {
        if (m_Body)
        {
            m_Body->setAngularVelocity(btVector3(vel.x, vel.y, vel.z));
            m_Body->activate(true);
        }
    }

    glm::vec3 GetLinearVelocity() const override
    {
        if (m_Body)
        {
            const btVector3& v = m_Body->getLinearVelocity();
            return glm::vec3(v.x(), v.y(), v.z());
        }
        return glm::vec3(0.0f);
    }

    glm::vec3 GetAngularVelocity() const override
    {
        if (m_Body)
        {
            const btVector3& v = m_Body->getAngularVelocity();
            return glm::vec3(v.x(), v.y(), v.z());
        }
        return glm::vec3(0.0f);
    }

    void ApplyCentralForce(const glm::vec3& force) override
    {
        if (m_Body)
            m_Body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }

    void ApplyTorque(const glm::vec3& torque) override
    {
        if (m_Body)
            m_Body->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }

    void SetFriction(float friction) override
    {
        if (m_Body)
            m_Body->setFriction(friction);
    }

    void SetRestitution(float restitution) override
    {
        if (m_Body)
            m_Body->setRestitution(restitution);
    }

    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override
    {
        if (m_Body)
        {
            btTransform tr;
            tr.setOrigin(btVector3(pos.x, pos.y, pos.z));
            tr.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
            m_Body->setWorldTransform(tr);
            if(m_Body->getMotionState())
                m_Body->getMotionState()->setWorldTransform(tr);
        }
    }

    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override
    {
        if (m_Body && m_Body->getMotionState())
        {
            btTransform tr;
            m_Body->getMotionState()->getWorldTransform(tr);
            btVector3 p = tr.getOrigin();
            btQuaternion r = tr.getRotation();
            pos = glm::vec3(p.x(), p.y(), p.z());
            rot = glm::quat(r.w(), r.x(), r.y(), r.z());
        }
    }

    void Activate(bool forceActivation) override
    {
        if (m_Body)
            m_Body->activate(forceActivation);
    }
    
    bool IsActive() const override
    {
        return m_Body ? m_Body->isActive() : false;
    }

    bool IsStatic() const override
    {
        return m_Body ? m_Body->isStaticObject() : false;
    }

    bool IsKinematic() const override
    {
        return m_Body ? m_Body->isKinematicObject() : false;
    }

    bool IsTrigger() const override
    {
        return m_Body ? (m_Body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) : false;
    }

    void SetKinematic(bool isKinematic) override
    {
        if (!m_Body) return;
        if (isKinematic)
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
            m_Body->setActivationState(DISABLE_DEACTIVATION);
        }
        else
        {
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
            m_Body->forceActivationState(ACTIVE_TAG);
        }
    }

    void SetStatic(bool isStatic) override
    {
        // Bullet handles static based on mass (0) usually, but we can force flags if needed
        if (!m_Body) return;
        if (isStatic)
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        else
            m_Body->setCollisionFlags(m_Body->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
    }

    void SetAlwaysActive(bool alwaysActive) override
    {
        if (!m_Body) return;
        if (alwaysActive)
            m_Body->setActivationState(DISABLE_DEACTIVATION);
        else
            m_Body->setActivationState(ACTIVE_TAG); // Or WANTS_DEACTIVATION
    }

    void SetUserPointer(void* ptr) override
    {
        if (m_Body)
            m_Body->setUserPointer(ptr);
    }

    void* GetUserPointer() const override
    {
        return m_Body ? m_Body->getUserPointer() : nullptr;
    }

    void SetLinearFactor(const glm::vec3& factor) override
    {
        if (m_Body)
            m_Body->setLinearFactor(btVector3(factor.x, factor.y, factor.z));
    }

    void SetAngularFactor(const glm::vec3& factor) override
    {
        if (m_Body)
            m_Body->setAngularFactor(btVector3(factor.x, factor.y, factor.z));
    }

    btRigidBody* GetRaw() const { return m_Body; }

private:
    btRigidBody* m_Body = nullptr;
    std::shared_ptr<ICollisionShape> m_Shape;
};
