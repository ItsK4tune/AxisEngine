

#ifndef BT_MULTIBODY_MLCP_CONSTRAINT_SOLVER_H
#define BT_MULTIBODY_MLCP_CONSTRAINT_SOLVER_H

#include "LinearMath/btMatrixX.h"
#include "LinearMath/btThreads.h"
#include "BulletDynamics/Featherstone/btMultiBodyConstraintSolver.h"

class btMLCPSolverInterface;
class btMultiBody;

class btMultiBodyMLCPConstraintSolver : public btMultiBodyConstraintSolver
{
protected:
	
	

	
	btMatrixXu m_A;

	
	btVectorXu m_b;

	
	btVectorXu m_x;

	
	btVectorXu m_lo;

	
	btVectorXu m_hi;

	

	
	
	

	
	btVectorXu m_bSplit;

	
	btVectorXu m_xSplit;

	

	
	

	
	btMatrixXu m_multiBodyA;

	
	btVectorXu m_multiBodyB;

	
	btVectorXu m_multiBodyX;

	
	btVectorXu m_multiBodyLo;

	
	btVectorXu m_multiBodyHi;

	

	
	
	
	
	
	
	btAlignedObjectArray<int> m_limitDependencies;

	
	
	
	
	
	
	btAlignedObjectArray<int> m_multiBodyLimitDependencies;

	
	btAlignedObjectArray<btSolverConstraint*> m_allConstraintPtrArray;

	
	btAlignedObjectArray<btMultiBodySolverConstraint*> m_multiBodyAllConstraintPtrArray;

	
	btMLCPSolverInterface* m_solver;

	
	int m_fallback;

	
	
	
	
	
	

	
	btMatrixXu m_scratchJ3;

	
	btMatrixXu m_scratchJInvM3;

	
	btAlignedObjectArray<int> m_scratchOfs;

	

	
	virtual void createMLCPFast(const btContactSolverInfo& infoGlobal);

	
	void createMLCPFastRigidBody(const btContactSolverInfo& infoGlobal);

	
	void createMLCPFastMultiBody(const btContactSolverInfo& infoGlobal);

	
	virtual bool solveMLCP(const btContactSolverInfo& infoGlobal);

	
	btScalar solveGroupCacheFriendlySetup(
		btCollisionObject** bodies,
		int numBodies,
		btPersistentManifold** manifoldPtr,
		int numManifolds,
		btTypedConstraint** constraints,
		int numConstraints,
		const btContactSolverInfo& infoGlobal,
		btIDebugDraw* debugDrawer) BT_OVERRIDE;

	
	btScalar solveGroupCacheFriendlyIterations(
		btCollisionObject** bodies,
		int numBodies,
		btPersistentManifold** manifoldPtr,
		int numManifolds,
		btTypedConstraint** constraints,
		int numConstraints,
		const btContactSolverInfo& infoGlobal,
		btIDebugDraw* debugDrawer) ;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR()

	
	
	
	
	explicit btMultiBodyMLCPConstraintSolver(btMLCPSolverInterface* solver);

	
	virtual ~btMultiBodyMLCPConstraintSolver();

	
	void setMLCPSolver(btMLCPSolverInterface* solver);

	
	
	int getNumFallbacks() const;

	
	void setNumFallbacks(int num);

	
	virtual btConstraintSolverType getSolverType() const;
};

#endif  
