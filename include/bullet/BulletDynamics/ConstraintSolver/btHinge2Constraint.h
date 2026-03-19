

#ifndef BT_HINGE2_CONSTRAINT_H
#define BT_HINGE2_CONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btTypedConstraint.h"
#include "btGeneric6DofSpring2Constraint.h"






ATTRIBUTE_ALIGNED16(class)
btHinge2Constraint : public btGeneric6DofSpring2Constraint
{
protected:
	btVector3 m_anchor;
	btVector3 m_axis1;
	btVector3 m_axis2;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	
	
	btHinge2Constraint(btRigidBody & rbA, btRigidBody & rbB, btVector3 & anchor, btVector3 & axis1, btVector3 & axis2);
	
	const btVector3& getAnchor() { return m_calculatedTransformA.getOrigin(); }
	const btVector3& getAnchor2() { return m_calculatedTransformB.getOrigin(); }
	const btVector3& getAxis1() { return m_axis1; }
	const btVector3& getAxis2() { return m_axis2; }
	btScalar getAngle1() { return getAngle(2); }
	btScalar getAngle2() { return getAngle(0); }
	
	void setUpperLimit(btScalar ang1max) { setAngularUpperLimit(btVector3(-1.f, 0.f, ang1max)); }
	void setLowerLimit(btScalar ang1min) { setAngularLowerLimit(btVector3(1.f, 0.f, ang1min)); }
};

#endif  
