

#ifndef BT_FIXED_CONSTRAINT_H
#define BT_FIXED_CONSTRAINT_H

#include "btGeneric6DofSpring2Constraint.h"

ATTRIBUTE_ALIGNED16(class)
btFixedConstraint : public btGeneric6DofSpring2Constraint
{
public:
	btFixedConstraint(btRigidBody & rbA, btRigidBody & rbB, const btTransform& frameInA, const btTransform& frameInB);

	virtual ~btFixedConstraint();
};

#endif  
