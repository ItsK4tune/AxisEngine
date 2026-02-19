

#ifndef BT_MULTIBODY_CONSTRAINT_SOLVER_H
#define BT_MULTIBODY_CONSTRAINT_SOLVER_H

#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "btMultiBodySolverConstraint.h"

#define DIRECTLY_UPDATE_VELOCITY_DURING_SOLVER_ITERATIONS

class btMultiBody;

#include "btMultiBodyConstraint.h"

ATTRIBUTE_ALIGNED16(class)
btMultiBodyConstraintSolver : public btSequentialImpulseConstraintSolver
{
protected:
	btMultiBodyConstraintArray m_multiBodyNonContactConstraints;

	btMultiBodyConstraintArray m_multiBodyNormalContactConstraints;
	btMultiBodyConstraintArray m_multiBodyFrictionContactConstraints;
	btMultiBodyConstraintArray m_multiBodyTorsionalFrictionContactConstraints;
	btMultiBodyConstraintArray m_multiBodySpinningFrictionContactConstraints;

	btMultiBodyJacobianData m_data;

	
	btMultiBodyConstraint** m_tmpMultiBodyConstraints;
	int m_tmpNumMultiBodyConstraints;

	btScalar resolveSingleConstraintRowGeneric(const btMultiBodySolverConstraint& c);

	
	btScalar resolveConeFrictionConstraintRows(const btMultiBodySolverConstraint& cA1, const btMultiBodySolverConstraint& cB);

	void convertContacts(btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal);

	btMultiBodySolverConstraint& addMultiBodyFrictionConstraint(const btVector3& normalAxis, const btScalar& appliedImpulse, btPersistentManifold* manifold, int frictionIndex, btManifoldPoint& cp, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0, btScalar cfmSlip = 0);

	btMultiBodySolverConstraint& addMultiBodyTorsionalFrictionConstraint(const btVector3& normalAxis, btPersistentManifold* manifold, int frictionIndex, btManifoldPoint& cp,
																		 btScalar combinedTorsionalFriction,
																		 btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0, btScalar cfmSlip = 0);

	btMultiBodySolverConstraint& addMultiBodySpinningFrictionConstraint(const btVector3& normalAxis, btPersistentManifold* manifold, int frictionIndex, btManifoldPoint& cp,
		btScalar combinedTorsionalFriction,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0, btScalar cfmSlip = 0);

	void setupMultiBodyJointLimitConstraint(btMultiBodySolverConstraint & constraintRow,
											btScalar * jacA, btScalar * jacB,
											btScalar penetration, btScalar combinedFrictionCoeff, btScalar combinedRestitutionCoeff,
											const btContactSolverInfo& infoGlobal);

	void setupMultiBodyContactConstraint(btMultiBodySolverConstraint & solverConstraint,
										 const btVector3& contactNormal,
                     const btScalar& appliedImpulse,
										 btManifoldPoint& cp,
                     const btContactSolverInfo& infoGlobal,
										 btScalar& relaxation,
										 bool isFriction, btScalar desiredVelocity = 0, btScalar cfmSlip = 0);

	
	void setupMultiBodyTorsionalFrictionConstraint(btMultiBodySolverConstraint & solverConstraint,
												   const btVector3& contactNormal,
												   btManifoldPoint& cp,
												   btScalar combinedTorsionalFriction,
												   const btContactSolverInfo& infoGlobal,
												   btScalar& relaxation,
												   bool isFriction, btScalar desiredVelocity = 0, btScalar cfmSlip = 0);

	void convertMultiBodyContact(btPersistentManifold * manifold, const btContactSolverInfo& infoGlobal);
	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	
	virtual btScalar solveSingleIteration(int iteration, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	void applyDeltaVee(btScalar * deltaV, btScalar impulse, int velocityIndex, int ndof);
	void writeBackSolverBodyToMultiBody(btMultiBodySolverConstraint & constraint, btScalar deltaTime);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	virtual btScalar solveGroup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifold, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& info, btIDebugDraw* debugDrawer, btDispatcher* dispatcher);
	virtual btScalar solveGroupCacheFriendlyFinish(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);

	virtual void solveMultiBodyGroup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifold, int numManifolds, btTypedConstraint** constraints, int numConstraints, btMultiBodyConstraint** multiBodyConstraints, int numMultiBodyConstraints, const btContactSolverInfo& info, btIDebugDraw* debugDrawer, btDispatcher* dispatcher);
};

#endif  
