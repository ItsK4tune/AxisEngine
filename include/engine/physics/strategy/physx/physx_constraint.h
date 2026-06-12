#pragma once
#include <physics/interface/i_constraint.h>
#include <PxPhysics.h>
#include <extensions/PxJoint.h>

class PhysXConstraint : public IConstraint
{
public:
    PhysXConstraint(physx::PxJoint* joint) : m_Joint(joint)
    {
    }

    ~PhysXConstraint() override
    {
        if (m_Joint)
        {
            m_Joint->release();
            m_Joint = nullptr;
        }
    }

    void SetBreakingImpulseThreshold(float threshold) override
    {
        if (m_Joint)
            m_Joint->setBreakForce(threshold, threshold);
    }

    float GetBreakingImpulseThreshold() const override
    {
        float force = 0.0f;
        float torque = 0.0f;
        if (m_Joint)
            m_Joint->getBreakForce(force, torque);
        return force;
    }

    float GetAppliedImpulse() const override
    {
        return 0.0f;
    }

    physx::PxJoint* GetRaw() const { return m_Joint; }

private:
    physx::PxJoint* m_Joint = nullptr;
};
