

#ifndef BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_MT_H
#define BT_SEQUENTIAL_IMPULSE_CONSTRAINT_SOLVER_MT_H

#include "btSequentialImpulseConstraintSolver.h"
#include "btBatchedConstraints.h"
#include "LinearMath/btThreads.h"


































ATTRIBUTE_ALIGNED16(class)
btSequentialImpulseConstraintSolverMt : public btSequentialImpulseConstraintSolver
{
public:
	virtual void solveGroupCacheFriendlySplitImpulseIterations(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer) BT_OVERRIDE;
	virtual btScalar solveSingleIteration(int iteration, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer) BT_OVERRIDE;
	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer) BT_OVERRIDE;
	virtual btScalar solveGroupCacheFriendlyFinish(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal) BT_OVERRIDE;

	
	struct btContactManifoldCachedInfo
	{
		static const int MAX_NUM_CONTACT_POINTS = 4;

		int numTouchingContacts;
		int solverBodyIds[2];
		int contactIndex;
		int rollingFrictionIndex;
		bool contactHasRollingFriction[MAX_NUM_CONTACT_POINTS];
		btManifoldPoint* contactPoints[MAX_NUM_CONTACT_POINTS];
	};
	
	struct JointParams
	{
		int m_solverConstraint;
		int m_solverBodyA;
		int m_solverBodyB;
	};
	void internalInitMultipleJoints(btTypedConstraint * *constraints, int iBegin, int iEnd);
	void internalConvertMultipleJoints(const btAlignedObjectArray<JointParams>& jointParamsArray, btTypedConstraint** constraints, int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);

	
	static bool s_allowNestedParallelForLoops;        
	static int s_minimumContactManifoldsForBatching;  
	static btBatchedConstraints::BatchingMethod s_contactBatchingMethod;
	static btBatchedConstraints::BatchingMethod s_jointBatchingMethod;
	static int s_minBatchSize;  
	static int s_maxBatchSize;

protected:
	static const int CACHE_LINE_SIZE = 64;

	btBatchedConstraints m_batchedContactConstraints;
	btBatchedConstraints m_batchedJointConstraints;
	int m_numFrictionDirections;
	bool m_useBatching;
	bool m_useObsoleteJointConstraints;
	btAlignedObjectArray<btContactManifoldCachedInfo> m_manifoldCachedInfoArray;
	btAlignedObjectArray<int> m_rollingFrictionIndexTable;  
	btSpinMutex m_bodySolverArrayMutex;
	char m_antiFalseSharingPadding[CACHE_LINE_SIZE];  
	btSpinMutex m_kinematicBodyUniqueIdToSolverBodyTableMutex;
	btAlignedObjectArray<char> m_scratchMemory;

	virtual void randomizeConstraintOrdering(int iteration, int numIterations);
	virtual btScalar resolveAllJointConstraints(int iteration);
	virtual btScalar resolveAllContactConstraints();
	virtual btScalar resolveAllContactFrictionConstraints();
	virtual btScalar resolveAllContactConstraintsInterleaved();
	virtual btScalar resolveAllRollingFrictionConstraints();

	virtual void setupBatchedContactConstraints();
	virtual void setupBatchedJointConstraints();
	virtual void convertJoints(btTypedConstraint * *constraints, int numConstraints, const btContactSolverInfo& infoGlobal) BT_OVERRIDE;
	virtual void convertContacts(btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal) BT_OVERRIDE;
	virtual void convertBodies(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal) BT_OVERRIDE;

	int getOrInitSolverBodyThreadsafe(btCollisionObject & body, btScalar timeStep);
	void allocAllContactConstraints(btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal);
	void setupAllContactConstraints(const btContactSolverInfo& infoGlobal);
	void randomizeBatchedConstraintOrdering(btBatchedConstraints * batchedConstraints);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSequentialImpulseConstraintSolverMt();
	virtual ~btSequentialImpulseConstraintSolverMt();

	btScalar resolveMultipleJointConstraints(const btAlignedObjectArray<int>& consIndices, int batchBegin, int batchEnd, int iteration);
	btScalar resolveMultipleContactConstraints(const btAlignedObjectArray<int>& consIndices, int batchBegin, int batchEnd);
	btScalar resolveMultipleContactSplitPenetrationImpulseConstraints(const btAlignedObjectArray<int>& consIndices, int batchBegin, int batchEnd);
	btScalar resolveMultipleContactFrictionConstraints(const btAlignedObjectArray<int>& consIndices, int batchBegin, int batchEnd);
	btScalar resolveMultipleContactRollingFrictionConstraints(const btAlignedObjectArray<int>& consIndices, int batchBegin, int batchEnd);
	btScalar resolveMultipleContactConstraintsInterleaved(const btAlignedObjectArray<int>& contactIndices, int batchBegin, int batchEnd);

	void internalCollectContactManifoldCachedInfo(btContactManifoldCachedInfo * cachedInfoArray, btPersistentManifold * *manifoldPtr, int numManifolds, const btContactSolverInfo& infoGlobal);
	void internalAllocContactConstraints(const btContactManifoldCachedInfo* cachedInfoArray, int numManifolds);
	void internalSetupContactConstraints(int iContactConstraint, const btContactSolverInfo& infoGlobal);
	void internalConvertBodies(btCollisionObject * *bodies, int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void internalWriteBackContacts(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void internalWriteBackJoints(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
	void internalWriteBackBodies(int iBegin, int iEnd, const btContactSolverInfo& infoGlobal);
};

#endif  
