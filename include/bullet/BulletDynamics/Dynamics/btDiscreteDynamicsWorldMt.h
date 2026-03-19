

#ifndef BT_DISCRETE_DYNAMICS_WORLD_MT_H
#define BT_DISCRETE_DYNAMICS_WORLD_MT_H

#include "btDiscreteDynamicsWorld.h"
#include "btSimulationIslandManagerMt.h"
#include "BulletDynamics/ConstraintSolver/btConstraintSolver.h"










class btConstraintSolverPoolMt : public btConstraintSolver
{
public:
	
	explicit btConstraintSolverPoolMt(int numSolvers);

	
	btConstraintSolverPoolMt(btConstraintSolver** solvers, int numSolvers);

	virtual ~btConstraintSolverPoolMt();

	
	virtual btScalar solveGroup(btCollisionObject** bodies,
								int numBodies,
								btPersistentManifold** manifolds,
								int numManifolds,
								btTypedConstraint** constraints,
								int numConstraints,
								const btContactSolverInfo& info,
								btIDebugDraw* debugDrawer,
								btDispatcher* dispatcher) BT_OVERRIDE;

	virtual void reset() BT_OVERRIDE;
	virtual btConstraintSolverType getSolverType() const BT_OVERRIDE { return m_solverType; }

private:
	const static size_t kCacheLineSize = 128;
	struct ThreadSolver
	{
		btConstraintSolver* solver;
		btSpinMutex mutex;
		char _cachelinePadding[kCacheLineSize - sizeof(btSpinMutex) - sizeof(void*)];  
	};
	btAlignedObjectArray<ThreadSolver> m_solvers;
	btConstraintSolverType m_solverType;

	ThreadSolver* getAndLockThreadSolver();
	void init(btConstraintSolver** solvers, int numSolvers);
};











ATTRIBUTE_ALIGNED16(class)
btDiscreteDynamicsWorldMt : public btDiscreteDynamicsWorld
{
protected:
	btConstraintSolver* m_constraintSolverMt;

	virtual void solveConstraints(btContactSolverInfo & solverInfo) BT_OVERRIDE;

	virtual void predictUnconstraintMotion(btScalar timeStep) BT_OVERRIDE;

	struct UpdaterCreatePredictiveContacts : public btIParallelForBody
	{
		btScalar timeStep;
		btRigidBody** rigidBodies;
		btDiscreteDynamicsWorldMt* world;

		void forLoop(int iBegin, int iEnd) const BT_OVERRIDE
		{
			world->createPredictiveContactsInternal(&rigidBodies[iBegin], iEnd - iBegin, timeStep);
		}
	};
	virtual void createPredictiveContacts(btScalar timeStep) BT_OVERRIDE;

	struct UpdaterIntegrateTransforms : public btIParallelForBody
	{
		btScalar timeStep;
		btRigidBody** rigidBodies;
		btDiscreteDynamicsWorldMt* world;

		void forLoop(int iBegin, int iEnd) const BT_OVERRIDE
		{
			world->integrateTransformsInternal(&rigidBodies[iBegin], iEnd - iBegin, timeStep);
		}
	};
	virtual void integrateTransforms(btScalar timeStep) BT_OVERRIDE;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btDiscreteDynamicsWorldMt(btDispatcher * dispatcher,
							  btBroadphaseInterface * pairCache,
							  btConstraintSolverPoolMt * solverPool,        
							  btConstraintSolver * constraintSolverMt,      
							  btCollisionConfiguration * collisionConfiguration);
	virtual ~btDiscreteDynamicsWorldMt();

	virtual int stepSimulation(btScalar timeStep, int maxSubSteps, btScalar fixedTimeStep) BT_OVERRIDE;
};

#endif  
