

#ifndef BT_SOLVER_CONSTRAINT_H
#define BT_SOLVER_CONSTRAINT_H

class btRigidBody;
#include "LinearMath/btVector3.h"
#include "LinearMath/btMatrix3x3.h"
#include "btJacobianEntry.h"
#include "LinearMath/btAlignedObjectArray.h"


#include "btSolverBody.h"


ATTRIBUTE_ALIGNED16(struct)
btSolverConstraint
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btVector3 m_relpos1CrossNormal;
	btVector3 m_contactNormal1;

	btVector3 m_relpos2CrossNormal;
	btVector3 m_contactNormal2;  

	btVector3 m_angularComponentA;
	btVector3 m_angularComponentB;

	mutable btSimdScalar m_appliedPushImpulse;
	mutable btSimdScalar m_appliedImpulse;

	btScalar m_friction;
	btScalar m_jacDiagABInv;
	btScalar m_rhs;
	btScalar m_cfm;

	btScalar m_lowerLimit;
	btScalar m_upperLimit;
	btScalar m_rhsPenetration;
	union {
		void* m_originalContactPoint;
		btScalar m_unusedPadding4;
		int m_numRowsForNonContactConstraint;
	};

	int m_overrideNumSolverIterations;
	int m_frictionIndex;
	int m_solverBodyIdA;
	int m_solverBodyIdB;

	enum btSolverConstraintType
	{
		BT_SOLVER_CONTACT_1D = 0,
		BT_SOLVER_FRICTION_1D
	};
};

typedef btAlignedObjectArray<btSolverConstraint> btConstraintArray;

#endif  
