


#ifndef BT_MLCP_SOLVER_H
#define BT_MLCP_SOLVER_H

#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "LinearMath/btMatrixX.h"
#include "BulletDynamics/MLCPSolvers/btMLCPSolverInterface.h"

class btMLCPSolver : public btSequentialImpulseConstraintSolver
{
protected:
	btMatrixXu m_A;
	btVectorXu m_b;
	btVectorXu m_x;
	btVectorXu m_lo;
	btVectorXu m_hi;

	
	btVectorXu m_bSplit;
	btVectorXu m_xSplit;
	btVectorXu m_bSplit1;
	btVectorXu m_xSplit2;

	btAlignedObjectArray<int> m_limitDependencies;
	btAlignedObjectArray<btSolverConstraint*> m_allConstraintPtrArray;
	btMLCPSolverInterface* m_solver;
	int m_fallback;

	
	
	
	btMatrixXu m_scratchJ3;
	btMatrixXu m_scratchJInvM3;
	btAlignedObjectArray<int> m_scratchOfs;
	btMatrixXu m_scratchMInv;
	btMatrixXu m_scratchJ;
	btMatrixXu m_scratchJTranspose;
	btMatrixXu m_scratchTmp;

	virtual btScalar solveGroupCacheFriendlySetup(btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);
	virtual btScalar solveGroupCacheFriendlyIterations(btCollisionObject** bodies, int numBodies, btPersistentManifold** manifoldPtr, int numManifolds, btTypedConstraint** constraints, int numConstraints, const btContactSolverInfo& infoGlobal, btIDebugDraw* debugDrawer);

	virtual void createMLCP(const btContactSolverInfo& infoGlobal);
	virtual void createMLCPFast(const btContactSolverInfo& infoGlobal);

	
	virtual bool solveMLCP(const btContactSolverInfo& infoGlobal);

public:
	btMLCPSolver(btMLCPSolverInterface* solver);
	virtual ~btMLCPSolver();

	void setMLCPSolver(btMLCPSolverInterface* solver)
	{
		m_solver = solver;
	}

	int getNumFallbacks() const
	{
		return m_fallback;
	}
	void setNumFallbacks(int num)
	{
		m_fallback = num;
	}

	virtual btConstraintSolverType getSolverType() const
	{
		return BT_MLCP_SOLVER;
	}
};

#endif  
