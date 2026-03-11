#include <physics/strategy/bullet/bullet_character_controller.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>

BulletCharacterController::BulletCharacterController(btPairCachingGhostObject* ghostObject, btKinematicCharacterController* controller, std::shared_ptr<ICollisionShape> shape)
    : m_GhostObject(ghostObject), m_Controller(controller), m_Shape(shape)
{
}

BulletCharacterController::~BulletCharacterController()
{
    delete m_Controller;
    delete m_GhostObject;
}

void BulletCharacterController::SetWalkDirection(const glm::vec3& dir)
{
    m_Controller->setWalkDirection(btVector3(dir.x, dir.y, dir.z));
}

void BulletCharacterController::SetVelocity(const glm::vec3& vel)
{
    m_Controller->setVelocityForTimeInterval(btVector3(vel.x, vel.y, vel.z), 1.0f);
}

void BulletCharacterController::Jump()
{
    m_Controller->jump();
}

bool BulletCharacterController::OnGround() const
{
    return m_Controller->onGround();
}

void BulletCharacterController::SetStepHeight(float height)
{
    m_Controller->setStepHeight(height);
}

void BulletCharacterController::SetMaxSlope(float slopeRadians)
{
    m_Controller->setMaxSlope(slopeRadians);
}

void BulletCharacterController::GetWorldTransform(glm::vec3& pos, glm::quat& rot) const
{
    btTransform trans = m_GhostObject->getWorldTransform();
    pos = glm::vec3(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
    btQuaternion q = trans.getRotation();
    rot = glm::quat(q.w(), q.x(), q.y(), q.z());
}

void BulletCharacterController::SetWorldTransform(const glm::vec3& pos, const glm::quat& rot)
{
    btTransform trans;
    trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
    trans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
    m_GhostObject->setWorldTransform(trans);
}

void BulletCharacterController::Activate(bool forceActivation)
{
    m_GhostObject->activate(forceActivation);
}

void BulletCharacterController::SetUserPointer(void* ptr)
{
    if (m_GhostObject)
        m_GhostObject->setUserPointer(ptr);
}

void* BulletCharacterController::GetUserPointer() const
{
    return m_GhostObject ? m_GhostObject->getUserPointer() : nullptr;
}
