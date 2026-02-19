



#ifndef BT_MULTIBODY_JOINT_MOTOR_H
#define BT_MULTIBODY_JOINT_MOTOR_H

#include "btMultiBodyConstraint.h"
struct btSolverInfo;

class btMultiBodyJointMotor : public btMultiBodyConstraint
{
protected:
	btScalar m_desiredVelocity;
	btScalar m_desiredPosition;
	btScalar m_kd;
	btScalar m_kp;
	btScalar m_erp;
	btScalar m_rhsClamp;  

public:
	btMultiBodyJointMotor(btMultiBody* body, int link, btScalar desiredVelocity, btScalar maxMotorImpulse);
	btMultiBodyJointMotor(btMultiBody* body, int link, int linkDoF, btScalar desiredVelocity, btScalar maxMotorImpulse);
	virtual ~btMultiBodyJointMotor();
	virtual void finalizeMultiDof();

	virtual int getIslandIdA() const;
	virtual int getIslandIdB() const;

	virtual void createConstraintRows(btMultiBodyConstraintArray& constraintRows,
									  btMultiBodyJacobianData& data,
									  const btContactSolverInfo& infoGlobal);

	virtual void setVelocityTarget(btScalar velTarget, btScalar kd = 1.f)
	{
		m_desiredVelocity = velTarget;
		m_kd = kd;
	}

	virtual void setPositionTarget(btScalar posTarget, btScalar kp = 1.f)
	{
		m_desiredPosition = posTarget;
		m_kp = kp;
	}

	virtual void setErp(btScalar erp)
	{
		m_erp = erp;
	}
	virtual btScalar getErp() const
	{
		return m_erp;
	}
	virtual void setRhsClamp(btScalar rhsClamp)
	{
		m_rhsClamp = rhsClamp;
	}
	virtual void debugDraw(class btIDebugDraw* drawer)
	{
		
	}
};

#endif  
