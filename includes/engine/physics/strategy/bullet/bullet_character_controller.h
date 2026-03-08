#pragma once

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <memory>
#include <physics/interface/i_character_controller.h>
#include <physics/strategy/bullet/bullet_collision_shape.h>

class BulletCharacterController : public ICharacterController
{
public:
    BulletCharacterController(btPairCachingGhostObject* ghostObject, btKinematicCharacterController* controller, std::shared_ptr<ICollisionShape> shape);
    ~BulletCharacterController();

    void SetWalkDirection(const glm::vec3& dir) override;
    void SetVelocity(const glm::vec3& vel) override;
    void Jump() override;
    bool OnGround() const override;

    void SetStepHeight(float height) override;
    void SetMaxSlope(float slopeRadians) override;

    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override;
    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override;

    void Activate(bool forceActivation = false) override;

    btKinematicCharacterController* GetRawController() const { return m_Controller; }
    btPairCachingGhostObject* GetGhostObject() const { return m_GhostObject; }

private:
    btPairCachingGhostObject* m_GhostObject;
    btKinematicCharacterController* m_Controller;
    std::shared_ptr<ICollisionShape> m_Shape;
};