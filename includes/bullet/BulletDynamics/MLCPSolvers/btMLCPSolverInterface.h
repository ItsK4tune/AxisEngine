


#ifndef BT_MLCP_SOLVER_INTERFACE_H
#define BT_MLCP_SOLVER_INTERFACE_H

#include "LinearMath/btMatrixX.h"

class btMLCPSolverInterface
{
public:
	virtual ~btMLCPSolverInterface()
	{
	}

	
	virtual bool solveMLCP(const btMatrixXu& A, const btVectorXu& b, btVectorXu& x, const btVectorXu& lo, const btVectorXu& hi, const btAlignedObjectArray<int>& limitDependency, int numIterations, bool useSparsity = true) = 0;
};

#endif  
