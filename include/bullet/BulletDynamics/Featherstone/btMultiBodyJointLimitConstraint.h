

#ifndef BT_MULTIBODY_JOINT_LIMIT_CONSTRAINT_H
#define BT_MULTIBODY_JOINT_LIMIT_CONSTRAINT_H

#include "btMultiBodyConstraint.h"
struct btSolverInfo;

class btMultiBodyJointLimitConstraint : public btMultiBodyConstraint
{
protected:
	btScalar m_lowerBound;
	btScalar m_upperBound;

public:
	btMultiBodyJointLimitConstraint(btMultiBody* body, int link, btScalar lower, btScalar upper);
	virtual ~btMultiBodyJointLimitConstraint();

	virtual void finalizeMultiDof();

	virtual int getIslandIdA() const;
	virtual int getIslandIdB() const;

	virtual void createConstraintRows(btMultiBodyConstraintArray& constraintRows,
									  btMultiBodyJacobianData& data,
									  const btContactSolverInfo& infoGlobal);

	virtual void debugDraw(class btIDebugDraw* drawer)
	{
		
	}
	btScalar getLowerBound() const
	{
		return m_lowerBound;
	}
	btScalar getUpperBound() const
	{
		return m_upperBound;
	}
	void setLowerBound(btScalar lower)
	{
		m_lowerBound = lower;
	}
	void setUpperBound(btScalar upper)
	{
		m_upperBound = upper;
	}
};

#endif  
