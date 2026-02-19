



#ifndef BT_MULTIBODY_FIXED_CONSTRAINT_H
#define BT_MULTIBODY_FIXED_CONSTRAINT_H

#include "btMultiBodyConstraint.h"

class btMultiBodyFixedConstraint : public btMultiBodyConstraint
{
protected:
	btRigidBody* m_rigidBodyA;
	btRigidBody* m_rigidBodyB;
	btVector3 m_pivotInA;
	btVector3 m_pivotInB;
	btMatrix3x3 m_frameInA;
	btMatrix3x3 m_frameInB;

public:
	btMultiBodyFixedConstraint(btMultiBody* body, int link, btRigidBody* bodyB, const btVector3& pivotInA, const btVector3& pivotInB, const btMatrix3x3& frameInA, const btMatrix3x3& frameInB);
	btMultiBodyFixedConstraint(btMultiBody* bodyA, int linkA, btMultiBody* bodyB, int linkB, const btVector3& pivotInA, const btVector3& pivotInB, const btMatrix3x3& frameInA, const btMatrix3x3& frameInB);

	virtual ~btMultiBodyFixedConstraint();

	virtual void finalizeMultiDof();

	virtual int getIslandIdA() const;
	virtual int getIslandIdB() const;

	virtual void createConstraintRows(btMultiBodyConstraintArray& constraintRows,
									  btMultiBodyJacobianData& data,
									  const btContactSolverInfo& infoGlobal);

	const btVector3& getPivotInA() const
	{
		return m_pivotInA;
	}

	void setPivotInA(const btVector3& pivotInA)
	{
		m_pivotInA = pivotInA;
	}

	const btVector3& getPivotInB() const
	{
		return m_pivotInB;
	}

	virtual void setPivotInB(const btVector3& pivotInB)
	{
		m_pivotInB = pivotInB;
	}

	const btMatrix3x3& getFrameInA() const
	{
		return m_frameInA;
	}

	void setFrameInA(const btMatrix3x3& frameInA)
	{
		m_frameInA = frameInA;
	}

	const btMatrix3x3& getFrameInB() const
	{
		return m_frameInB;
	}

	virtual void setFrameInB(const btMatrix3x3& frameInB)
	{
		m_frameInB = frameInB;
	}

	virtual void debugDraw(class btIDebugDraw* drawer);
};

#endif  
