

#ifndef BT_CONSTRAINT_SOLVER_H
#define BT_CONSTRAINT_SOLVER_H

#include "LinearMath/btScalar.h"

class btPersistentManifold;
class btRigidBody;
class btCollisionObject;
class btTypedConstraint;
struct btContactSolverInfo;
struct btBroadphaseProxy;
class btIDebugDraw;
class btStackAlloc;
class btDispatcher;


enum btConstraintSolverType
{
	BT_SEQUENTIAL_IMPULSE_SOLVER = 1,
	BT_MLCP_SOLVER = 2,
	BT_NNCG_SOLVER = 4,
	BT_MULTIBODY_SOLVER = 8,
	BT_BLOCK_SOLVER = 16,
};

class btConstraintSolver
{
public:
	virtual ~btConstraintSolver() {}

	virtual void prepareSolve(int , int ) { ; }

	
	virtual btScalar solveGroup(btCollisionObject** bodies, int numBodies, btPersistentManifold** manifold, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& info, class btIDebugDraw* debugDrawer, btDispatcher* dispatcher) = 0;

	virtual void allSolved(const btContactSolverInfo& , class btIDebugDraw* ) { ; }

	
	virtual void reset() = 0;

	virtual btConstraintSolverType getSolverType() const = 0;
};

#endif  
