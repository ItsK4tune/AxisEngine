



#ifndef BT_MULTIBODY_POINT2POINT_H
#define BT_MULTIBODY_POINT2POINT_H

#include "btMultiBodyConstraint.h"



ATTRIBUTE_ALIGNED16(class)
btMultiBodyPoint2Point : public btMultiBodyConstraint
{
protected:
	btRigidBody* m_rigidBodyA;
	btRigidBody* m_rigidBodyB;
	btVector3 m_pivotInA;
	btVector3 m_pivotInB;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMultiBodyPoint2Point(btMultiBody * body, int link, btRigidBody* bodyB, const btVector3& pivotInA, const btVector3& pivotInB);
	btMultiBodyPoint2Point(btMultiBody * bodyA, int linkA, btMultiBody* bodyB, int linkB, const btVector3& pivotInA, const btVector3& pivotInB);

	virtual ~btMultiBodyPoint2Point();

	virtual void finalizeMultiDof();

	virtual int getIslandIdA() const;
	virtual int getIslandIdB() const;

	virtual void createConstraintRows(btMultiBodyConstraintArray & constraintRows,
									  btMultiBodyJacobianData & data,
									  const btContactSolverInfo& infoGlobal);

	const btVector3& getPivotInB() const
	{
		return m_pivotInB;
	}

	virtual void setPivotInB(const btVector3& pivotInB)
	{
		m_pivotInB = pivotInB;
	}

	virtual void debugDraw(class btIDebugDraw * drawer);
};

#endif  
