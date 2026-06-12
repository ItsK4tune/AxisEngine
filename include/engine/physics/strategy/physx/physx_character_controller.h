#pragma once
#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_collision_shape.h>
#include <characterkinematic/PxController.h>
#include <memory>

class PhysXCharacterController : public ICharacterController
{
public:
    PhysXCharacterController(physx::PxController* controller, std::shared_ptr<ICollisionShape> shape)
        : m_Controller(controller), m_Shape(shape)
    {
    }

    ~PhysXCharacterController() override
    {
        if (m_Controller)
        {
            m_Controller->release();
            m_Controller = nullptr;
        }
    }

    void SetWalkDirection(const glm::vec3& dir) override;
    void SetVelocity(const glm::vec3& vel, float timeInterval) override;
    void SetFallSpeed(float speed) override;
    void SetGravity(const glm::vec3& gravity) override;
    void SetJumpSpeed(float speed) override;
    void Jump() override;
    bool OnGround() const override;

    void SetStepHeight(float height) override;
    void SetMaxSlope(float slopeRadians) override;

    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override;
    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override;

    void Activate(bool forceActivation = false) override;

    void SetUserPointer(void* ptr) override;
    void* GetUserPointer() const override;

    void Update(float dt);

    physx::PxController* GetRawController() const { return m_Controller; }
    std::shared_ptr<ICollisionShape> GetShape() const { return m_Shape; }

private:
    physx::PxController* m_Controller = nullptr;
    std::shared_ptr<ICollisionShape> m_Shape;
    bool m_OnGround = false;
    glm::vec3 m_WalkDirection{0.0f};
    glm::vec3 m_Gravity{0.0f, -9.81f, 0.0f};
    float m_JumpSpeed = 10.0f;
    float m_FallSpeed = 55.0f;
    float m_VerticalVelocity = 0.0f;
};
