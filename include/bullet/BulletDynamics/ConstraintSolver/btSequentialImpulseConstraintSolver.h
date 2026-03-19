

#ifndef BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_H
#define BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_H

class btIDebugDraw;
class btPersistentManifold;
class btDispatcher;
class btCollisionObject;
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"
#include "BulletDynamics/ConstraintSolver/btSolverBody.h"
#include "BulletDynamics/ConstraintSolver/btSolverConstraint.h"
#include "BulletCollision/NarrowPhaseCollision/btManifoldPoint.h"
#include "BulletDynamics/ConstraintSolver/btConstraintSolver.h"

typedef btScalar (*btSingleConstraintRowSolver)(btSolverBody&, btSolverBody&, const btSolverConstraint&);

struct btSolverAnalyticsData
{
	btSolverAnalyticsData()
	{
		m_numSolverCalls = 0;
		m_numIterationsUsed = -1;
		m_remainingLeastSquaresResidual = -1;
		m_islandId = -2;
	}
	int m_islandId;
	int m_numBodies;
	int m_numContactManifolds;
	int m_numSolverCalls;
	int m_numIterationsUsed;
	double m_remainingLeastSquaresResidual;
};


ATTRIBUTE_ALIGNED16(class)
btSequentialImpulseConstraintSolver : public btConstraintSolver
{
	

protected:
	btAlignedObjectArray<btSolverBody> m_tmpSolverBodyPool;
	btConstraintArray m_tmpSolverContactConstraintPool;
	btConstraintArray m_tmpSolverNonContactConstraintPool;
	btConstraintArray m_tmpSolverContactFrictionConstraintPool;
	btConstraintArray m_tmpSolverContactRollingFrictionConstraintPool;

	btAlignedObjectArray<int> m_orderTmpConstraintPool;
	btAlignedObjectArray<int> m_orderNonContactConstraintPool;
	btAlignedObjectArray<int> m_orderFrictionConstraintPool;
	btAlignedObjectArray<btTypedConstraint::btConstraintInfo1> m_tmpConstraintSizesPool;
	int m_maxOverrideNumSolverIterations;
	int m_fixedBodyId;
	
	
	
	
	
	
	
	btAlignedObjectArray<int> m_kinematicBodyUniqueIdToSolverBodyTable;  

	btSingleConstraintRowSolver m_resolveSingleConstraintRowGeneric;
	btSingleConstraintRowSolver m_resolveSingleConstraintRowLowerLimit;
	btSingleConstraintRowSolver m_resolveSplitPenetrationImpulse;
	int m_cachedSolverMode;  
	void setupSolverFunctions(bool useSimd);

	btScalar m_leastSquaresResidual;

	void setupFrictionConstraint(btSolverConstraint & solverConstraint, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation,
		const btContactSolverInfo& infoGlobal,
		btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);

	void setupTorsionalFrictionConstraint(btSolverConstraint & solverConstraint, const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, btScalar combinedTorsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2,
		btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation,
		btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);

	btSolverConstraint& addFrictionConstraint(const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, const btContactSolverInfo& infoGlobal, btScalar desiredVelocity = 0., btScalar cfmSlip = 0.);
	btSolverConstraint& addTorsionalFrictionConstraint(const btVector3& normalAxis, int solverBodyIdA, int solverBodyIdB, int frictionIndex, btManifoldPoint& cp, btScalar torsionalFriction, const btVector3& rel_pos1, const btVector3& rel_pos2, btCollisionObject* colObj0, btCollisionObject* colObj1, btScalar relaxation, btScalar desiredVelocity = 0, btScalar cfmSlip = 0.f);

	void setupContactConstraint(btSolverConstraint & solverConstraint, int solverBodyIdA, int solverBodyIdB, btManifoldPoint& cp,
		const btContactSolverInfo& infoGlobal, btScalar& relaxation, const btVector3& rel_pos1, const btVector3& rel_pos2);

	static void applyAnisotropicFriction(btCollisionObject * colObj, btVector3 & frictionDirection, int frictionMode);

	void setFrictionConstraintImpulse(btSolverConstraint & solverConstraint, int solverBodyIdA, int solverBodyIdB,
		btManifoldPoint& cp, const btContactSolverInfo& infoGlobal);

	
	unsigned long m_btSeed2;

	btScalar restitutionCurve(btScalar rel_vel, btScalar restitution, btScalar velocityThreshold);

	virtual void convertContacts(btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal);

	void convertContact(btPersistentManifold * manifold, const btContactSolverInfo& infoGlobal);

	virtual void convertJoints(btTypedConstraint * *constraints, int numConstraints, const btContactSolverInfo& infoGlobal);
	void convertJoint(btSolverConstraint * currentConstraintRow, btTypedConstraint * constraint, const btTypedConstraint::btConstraintInfo1& info1, int solverBodyIdA, int solverBodyIdB, const btContactSolverInfo& infoGlobal);

	virtual void convertBodies(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);

	btScalar resolveSplitPenetrationSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

	btScalar resolveSplitPenetrationImpulseCacheFriendly(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

	
	int getOrInitSolverBody(btCollisionObject & body, btScalar timeStep);
	void initSolverBody(btSolverBody * solverBody, btCollisionObject * collisionObject, btScalar timeStep);

	btScalar resolveSingleConstraintRowGeneric(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowGenericSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowLowerLimit(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSingleConstraintRowLowerLimitSIMD(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint);
	btScalar resolveSplitPenetrationImpulse(btSolverBody & bodyA, btSolverBody & bodyB, const btSolverConstraint& contactConstraint)
	{
		return m_resolveSplitPenetrationImpulse(bodyA, bodyB, contactConstraint);
	}

protected:
	void writeBackContacts(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void writeBackJoints(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void writeBackBodies(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	virtual void solveGroupCacheFriendlySplitImpulseIterations(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	virtual btScalar solveGroupCacheFriendlyFinish(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);
	virtual btScalar solveSingleIteration(int iteration, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	virtual btScalar solveGroupCacheFriendlyIterations(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSequentialImpulseConstraintSolver();
	virtual ~btSequentialImpulseConstraintSolver();

	virtual btScalar solveGroup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifold, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& info, btIDebugDraw* debugDrawer, btDispatcher* dispatcher);

	
	virtual void reset();

	unsigned long btRand2();

	int btRandInt2(int n);

	void setRandSeed(unsigned long seed)
	{
		m_btSeed2 = seed;
	}
	unsigned long getRandSeed() const
	{
		return m_btSeed2;
	}

	virtual btConstraintSolverType getSolverType() const
	{
		return BT_SEQUENTIAL_IMPULSE_SOLVER;
	}

	btSingleConstraintRowSolver getActiveConstraintRowSolverGeneric()
	{
		return m_resolveSingleConstraintRowGeneric;
	}
	void setConstraintRowSolverGeneric(btSingleConstraintRowSolver rowSolver)
	{
		m_resolveSingleConstraintRowGeneric = rowSolver;
	}
	btSingleConstraintRowSolver getActiveConstraintRowSolverLowerLimit()
	{
		return m_resolveSingleConstraintRowLowerLimit;
	}
	void setConstraintRowSolverLowerLimit(btSingleConstraintRowSolver rowSolver)
	{
		m_resolveSingleConstraintRowLowerLimit = rowSolver;
	}



	
	btSingleConstraintRowSolver getScalarConstraintRowSolverGeneric();
	btSingleConstraintRowSolver getSSE2ConstraintRowSolverGeneric();
	btSingleConstraintRowSolver getSSE4_1ConstraintRowSolverGeneric();

	
	btSingleConstraintRowSolver getScalarConstraintRowSolverLowerLimit();
	btSingleConstraintRowSolver getSSE2ConstraintRowSolverLowerLimit();
	btSingleConstraintRowSolver getSSE4_1ConstraintRowSolverLowerLimit();
	btSolverAnalyticsData m_analyticsData;
};

#endif  
