#pragma once

#include <physics/interface/i_constraint.h>
#include <btBulletDynamicsCommon.h>

class BulletConstraint : public IConstraint
{
public:
    BulletConstraint(btTypedConstraint* constraint) : m_Constraint(constraint)
    {
    }

    ~BulletConstraint()
    {
        if (m_Constraint)
        {
            delete m_Constraint;
            m_Constraint = nullptr;
        }
    }

    btTypedConstraint* GetRaw() const
    {
        return m_Constraint;
    }

    void SetBreakingImpulseThreshold(float threshold) override
    {
        if (m_Constraint)
        {
            m_Constraint->setBreakingImpulseThreshold(threshold);
        }
    }

    float GetBreakingImpulseThreshold() const override
    {
        return m_Constraint ? m_Constraint->getBreakingImpulseThreshold() : 0.0f;
    }

    float GetAppliedImpulse() const override
    {
        return m_Constraint ? m_Constraint->getAppliedImpulse() : 0.0f;
    }

private:
    btTypedConstraint* m_Constraint = nullptr;
};
