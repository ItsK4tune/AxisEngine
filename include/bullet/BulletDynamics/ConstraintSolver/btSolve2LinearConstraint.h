

#ifndef BT_SOLVE_2LINEAR_CONSTRAINT_H
#define BT_SOLVE_2LINEAR_CONSTRAINT_H

#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btVector3.h"

class btRigidBody;


class btSolve2LinearConstraint
{
	btScalar m_tau;
	btScalar m_damping;

public:
	btSolve2LinearConstraint(btScalar tau, btScalar damping)
	{
		m_tau = tau;
		m_damping = damping;
	}
	
	
	
	void resolveUnilateralPairConstraint(
		btRigidBody* body0,
		btRigidBody* body1,

		const btMatrix3x3& world2A,
		const btMatrix3x3& world2B,

		const btVector3& invInertiaADiag,
		const btScalar invMassA,
		const btVector3& linvelA, const btVector3& angvelA,
		const btVector3& rel_posA1,
		const btVector3& invInertiaBDiag,
		const btScalar invMassB,
		const btVector3& linvelB, const btVector3& angvelB,
		const btVector3& rel_posA2,

		btScalar depthA, const btVector3& normalA,
		const btVector3& rel_posB1, const btVector3& rel_posB2,
		btScalar depthB, const btVector3& normalB,
		btScalar& imp0, btScalar& imp1);

	
	
	
	void resolveBilateralPairConstraint(
		btRigidBody* body0,
		btRigidBody* body1,
		const btMatrix3x3& world2A,
		const btMatrix3x3& world2B,

		const btVector3& invInertiaADiag,
		const btScalar invMassA,
		const btVector3& linvelA, const btVector3& angvelA,
		const btVector3& rel_posA1,
		const btVector3& invInertiaBDiag,
		const btScalar invMassB,
		const btVector3& linvelB, const btVector3& angvelB,
		const btVector3& rel_posA2,

		btScalar depthA, const btVector3& normalA,
		const btVector3& rel_posB1, const btVector3& rel_posB2,
		btScalar depthB, const btVector3& normalB,
		btScalar& imp0, btScalar& imp1);

	
};

#endif  
