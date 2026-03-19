

#ifndef BT_UNIVERSAL_CONSTRAINT_H
#define BT_UNIVERSAL_CONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btTypedConstraint.h"
#include "btGeneric6DofConstraint.h"








ATTRIBUTE_ALIGNED16(class)
btUniversalConstraint : public btGeneric6DofConstraint
{
protected:
	btVector3 m_anchor;
	btVector3 m_axis1;
	btVector3 m_axis2;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	
	
	btUniversalConstraint(btRigidBody & rbA, btRigidBody & rbB, const btVector3& anchor, const btVector3& axis1, const btVector3& axis2);
	
	const btVector3& getAnchor() { return m_calculatedTransformA.getOrigin(); }
	const btVector3& getAnchor2() { return m_calculatedTransformB.getOrigin(); }
	const btVector3& getAxis1() { return m_axis1; }
	const btVector3& getAxis2() { return m_axis2; }
	btScalar getAngle1() { return getAngle(2); }
	btScalar getAngle2() { return getAngle(1); }
	
	void setUpperLimit(btScalar ang1max, btScalar ang2max) { setAngularUpperLimit(btVector3(0.f, ang1max, ang2max)); }
	void setLowerLimit(btScalar ang1min, btScalar ang2min) { setAngularLowerLimit(btVector3(0.f, ang1min, ang2min)); }

	void setAxis(const btVector3& axis1, const btVector3& axis2);
};

#endif  
