

#include "btDiscreteDynamicsWorldMt.h"


#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btSimpleBroadphase.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "btSimulationIslandManagerMt.h"
#include "LinearMath/btTransformUtil.h"
#include "LinearMath/btQuickprof.h"


#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.h"
#include "BulletDynamics/ConstraintSolver/btSliderConstraint.h"
#include "BulletDynamics/ConstraintSolver/btContactConstraint.h"

#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"

#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "LinearMath/btQuickprof.h"
#include "LinearMath/btMotionState.h"

#include "LinearMath/btSerializer.h"





btConstraintSolverPoolMt::ThreadSolver* btConstraintSolverPoolMt::getAndLockThreadSolver()
{
	int i = 0;
#if BT_THREADSAFE
	i = btGetCurrentThreadIndex() % m_solvers.size();
#endif  
	while (true)
	{
		ThreadSolver& solver = m_solvers[i];
		if (solver.mutex.tryLock())
		{
			return &solver;
		}
		
		i = (i + 1) % m_solvers.size();
	}
	return NULL;
}

void btConstraintSolverPoolMt::init(btConstraintSolver** solvers, int numSolvers)
{
	m_solverType = BT_SEQUENTIAL_IMPULSE_SOLVER;
	m_solvers.resize(numSolvers);
	for (int i = 0; i < numSolvers; ++i)
	{
		m_solvers[i].solver = solvers[i];
	}
	if (numSolvers > 0)
	{
		m_solverType = solvers[0]->getSolverType();
	}
}


btConstraintSolverPoolMt::btConstraintSolverPoolMt(int numSolvers)
{
	btAlignedObjectArray<btConstraintSolver*> solvers;
	solvers.reserve(numSolvers);
	for (int i = 0; i < numSolvers; ++i)
	{
		btConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
		solvers.push_back(solver);
	}
	init(&solvers[0], numSolvers);
}


btConstraintSolverPoolMt::btConstraintSolverPoolMt(btConstraintSolver** solvers, int numSolvers)
{
	init(solvers, numSolvers);
}

btConstraintSolverPoolMt::~btConstraintSolverPoolMt()
{
	
	for (int i = 0; i < m_solvers.size(); ++i)
	{
		ThreadSolver& solver = m_solvers[i];
		delete solver.solver;
		solver.solver = NULL;
	}
}


btScalar btConstraintSolverPoolMt::solveGroup(btCollisionObject** bodies,
											  int numBodies,
											  btPersistentManifold** manifolds,
											  int numManifolds,
											  btTypedConstraint** constraints,
											  int numConstraints,
											  const btContactSolverInfo& info,
											  btIDebugDraw* debugDrawer,
											  btDispatcher* dispatcher)
{
	ThreadSolver* ts = getAndLockThreadSolver();
	ts->solver->solveGroup(bodies, numBodies, manifolds, numManifolds, constraints, numConstraints, info, debugDrawer, dispatcher);
	ts->mutex.unlock();
	return 0.0f;
}

void btConstraintSolverPoolMt::reset()
{
	for (int i = 0; i < m_solvers.size(); ++i)
	{
		ThreadSolver& solver = m_solvers[i];
		solver.mutex.lock();
		solver.solver->reset();
		solver.mutex.unlock();
	}
}





btDiscreteDynamicsWorldMt::btDiscreteDynamicsWorldMt(btDispatcher* dispatcher,
													 btBroadphaseInterface* pairCache,
													 btConstraintSolverPoolMt* solverPool,
													 btConstraintSolver* constraintSolverMt,
													 btCollisionConfiguration* collisionConfiguration)
	: btDiscreteDynamicsWorld(dispatcher, pairCache, solverPool, collisionConfiguration)
{
	if (m_ownsIslandManager)
	{
		m_islandManager->~btSimulationIslandManager();
		btAlignedFree(m_islandManager);
	}
	{
		void* mem = btAlignedAlloc(sizeof(btSimulationIslandManagerMt), 16);
		btSimulationIslandManagerMt* im = new (mem) btSimulationIslandManagerMt();
		im->setMinimumSolverBatchSize(m_solverInfo.m_minimumSolverBatchSize);
		m_islandManager = im;
	}
	m_constraintSolverMt = constraintSolverMt;
}

btDiscreteDynamicsWorldMt::~btDiscreteDynamicsWorldMt()
{
}

void btDiscreteDynamicsWorldMt::solveConstraints(btContactSolverInfo& solverInfo)
{
	BT_PROFILE("solveConstraints");

	m_constraintSolver->prepareSolve(getCollisionWorld()->getNumCollisionObjects(), getCollisionWorld()->getDispatcher()->getNumManifolds());

	
	btSimulationIslandManagerMt* im = static_cast<btSimulationIslandManagerMt*>(m_islandManager);
	btSimulationIslandManagerMt::SolverParams solverParams;
	solverParams.m_solverPool = m_constraintSolver;
	solverParams.m_solverMt = m_constraintSolverMt;
	solverParams.m_solverInfo = &solverInfo;
	solverParams.m_debugDrawer = m_debugDrawer;
	solverParams.m_dispatcher = getCollisionWorld()->getDispatcher();
	im->buildAndProcessIslands(getCollisionWorld()->getDispatcher(), getCollisionWorld(), m_constraints, solverParams);

	m_constraintSolver->allSolved(solverInfo, m_debugDrawer);
}

struct UpdaterUnconstrainedMotion : public btIParallelForBody
{
	btScalar timeStep;
	btRigidBody** rigidBodies;

	void forLoop(int iBegin, int iEnd) const BT_OVERRIDE
	{
		for (int i = iBegin; i < iEnd; ++i)
		{
			btRigidBody* body = rigidBodies[i];
			if (!body->isStaticOrKinematicObject())
			{
				
				body->applyDamping(timeStep);
				body->predictIntegratedTransform(timeStep, body->getInterpolationWorldTransform());
			}
		}
	}
};

void btDiscreteDynamicsWorldMt::predictUnconstraintMotion(btScalar timeStep)
{
	BT_PROFILE("predictUnconstraintMotion");
	if (m_nonStaticRigidBodies.size() > 0)
	{
		UpdaterUnconstrainedMotion update;
		update.timeStep = timeStep;
		update.rigidBodies = &m_nonStaticRigidBodies[0];
		int grainSize = 50;  
		btParallelFor(0, m_nonStaticRigidBodies.size(), grainSize, update);
	}
}

void btDiscreteDynamicsWorldMt::createPredictiveContacts(btScalar timeStep)
{
	BT_PROFILE("createPredictiveContacts");
	releasePredictiveContacts();
	if (m_nonStaticRigidBodies.size() > 0)
	{
		UpdaterCreatePredictiveContacts update;
		update.world = this;
		update.timeStep = timeStep;
		update.rigidBodies = &m_nonStaticRigidBodies[0];
		int grainSize = 50;  
		btParallelFor(0, m_nonStaticRigidBodies.size(), grainSize, update);
	}
}

void btDiscreteDynamicsWorldMt::integrateTransforms(btScalar timeStep)
{
	BT_PROFILE("integrateTransforms");
	if (m_nonStaticRigidBodies.size() > 0)
	{
		UpdaterIntegrateTransforms update;
		update.world = this;
		update.timeStep = timeStep;
		update.rigidBodies = &m_nonStaticRigidBodies[0];
		int grainSize = 50;  
		btParallelFor(0, m_nonStaticRigidBodies.size(), grainSize, update);
	}
}

int btDiscreteDynamicsWorldMt::stepSimulation(btScalar timeStep, int maxSubSteps, btScalar fixedTimeStep)
{
	int numSubSteps = btDiscreteDynamicsWorld::stepSimulation(timeStep, maxSubSteps, fixedTimeStep);
	if (btITaskScheduler* scheduler = btGetTaskScheduler())
	{
		
		scheduler->sleepWorkerThreadsHint();
	}
	return numSubSteps;
}
