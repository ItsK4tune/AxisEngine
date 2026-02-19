

#ifndef BT_NNCG_CONSTRAINT_SOLVER_H
#define BT_NNCG_CONSTRAINT_SOLVER_H

#include "btSequentialImpulseConstraintSolver.h"

ATTRIBUTE_ALIGNED16(class)
btNNCGConstraintSolver : public btSequentialImpulseConstraintSolver
{
protected:
	btScalar m_deltafLengthSqrPrev;

	btAlignedObjectArray<btScalar> m_pNC;   
	btAlignedObjectArray<btScalar> m_pC;    
	btAlignedObjectArray<btScalar> m_pCF;   
	btAlignedObjectArray<btScalar> m_pCRF;  

	
	btAlignedObjectArray<btScalar> m_deltafNC;   
	btAlignedObjectArray<btScalar> m_deltafC;    
	btAlignedObjectArray<btScalar> m_deltafCF;   
	btAlignedObjectArray<btScalar> m_deltafCRF;  

protected:
	virtual btScalar solveGroupCacheFriendlyFinish(btCollisionObject * *bodies, int numBodies, const btContactSolverInfo& infoGlobal);
	virtual btScalar solveSingleIteration(int iteration, btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject * *bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btNNCGConstraintSolver() : btSequentialImpulseConstraintSolver(), m_onlyForNoneContact(false) {}

	virtual btConstraintSolverType getSolverType() const
	{
		return BT_NNCG_SOLVER;
	}

	bool m_onlyForNoneContact;
};

#endif  
