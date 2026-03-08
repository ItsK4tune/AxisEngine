#include <physics/strategy/bullet/bullet_rigid_body.h>

BulletRigidBody::~BulletRigidBody()
{
    if (m_Body)
    {
        if (m_Body->getMotionState()) delete m_Body->getMotionState();
        delete m_Body;
    }
}

void BulletRigidBody::SetLinearVelocity(const glm::vec3& vel)
{
    if (m_Body)
    {
        m_Body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
        m_Body->activate(true);
    }
}

void BulletRigidBody::SetAngularVelocity(const glm::vec3& vel)
{
    if (m_Body)
    {
        m_Body->setAngularVelocity(btVector3(vel.x, vel.y, vel.z));
        m_Body->activate(true);
    }
}

glm::vec3 BulletRigidBody::GetLinearVelocity() const
{
    if (m_Body)
    {
        const btVector3& v = m_Body->getLinearVelocity();
        return glm::vec3(v.x(), v.y(), v.z());
    }
    return glm::vec3(0.0f);
}

glm::vec3 BulletRigidBody::GetAngularVelocity() const
{
    if (m_Body)
    {
        const btVector3& v = m_Body->getAngularVelocity();
        return glm::vec3(v.x(), v.y(), v.z());
    }
    return glm::vec3(0.0f);
}

void BulletRigidBody::ApplyCentralForce(const glm::vec3& force)
{
    if (m_Body)
        m_Body->applyCentralForce(btVector3(force.x, force.y, force.z));
}

void BulletRigidBody::ApplyTorque(const glm::vec3& torque)
{
    if (m_Body)
        m_Body->applyTorque(btVector3(torque.x, torque.y, torque.z));
}

void BulletRigidBody::SetFriction(float friction)
{
    if (m_Body)
        m_Body->setFriction(friction);
}

void BulletRigidBody::SetRestitution(float restitution)
{
    if (m_Body)
        m_Body->setRestitution(restitution);
}

void BulletRigidBody::SetWorldTransform(const glm::vec3& pos, const glm::quat& rot)
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

void BulletRigidBody::GetWorldTransform(glm::vec3& pos, glm::quat& rot) const
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

void BulletRigidBody::Activate(bool forceActivation)
{
    if (m_Body)
        m_Body->activate(forceActivation);
}

bool BulletRigidBody::IsActive() const
{
    return m_Body ? m_Body->isActive() : false;
}

bool BulletRigidBody::IsStatic() const
{
    return m_Body ? m_Body->isStaticObject() : false;
}

bool BulletRigidBody::IsKinematic() const
{
    return m_Body ? m_Body->isKinematicObject() : false;
}

bool BulletRigidBody::IsTrigger() const
{
    return m_Body ? (m_Body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) : false;
}

void BulletRigidBody::SetKinematic(bool isKinematic)
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

void BulletRigidBody::SetStatic(bool isStatic)
{
    if (!m_Body) return;
    if (isStatic)
        m_Body->setCollisionFlags(m_Body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    else
        m_Body->setCollisionFlags(m_Body->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
}

void BulletRigidBody::SetAlwaysActive(bool alwaysActive)
{
    if (!m_Body) return;
    if (alwaysActive)
        m_Body->setActivationState(DISABLE_DEACTIVATION);
    else
        m_Body->setActivationState(ACTIVE_TAG);
}

void BulletRigidBody::SetUserPointer(void* ptr)
{
    if (m_Body)
        m_Body->setUserPointer(ptr);
}

void* BulletRigidBody::GetUserPointer() const
{
    return m_Body ? m_Body->getUserPointer() : nullptr;
}

void BulletRigidBody::SetLinearFactor(const glm::vec3& factor)
{
    if (m_Body)
        m_Body->setLinearFactor(btVector3(factor.x, factor.y, factor.z));
}

void BulletRigidBody::SetAngularFactor(const glm::vec3& factor)
{
    if (m_Body)
        m_Body->setAngularFactor(btVector3(factor.x, factor.y, factor.z));
}
