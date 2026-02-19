


#ifndef BT_PATH_SOLVER_H
#define BT_PATH_SOLVER_H


#ifdef BT_USE_PATH

extern "C"
{
#include "PATH/SimpleLCP.h"
#include "PATH/License.h"
#include "PATH/Error_Interface.h"
};
void __stdcall MyError(Void *data, Char *msg)
{
	printf("Path Error: %s\n", msg);
}
void __stdcall MyWarning(Void *data, Char *msg)
{
	printf("Path Warning: %s\n", msg);
}

Error_Interface e;

#include "btMLCPSolverInterface.h"
#include "Dantzig/lcp.h"

class btPathSolver : public btMLCPSolverInterface
{
public:
	btPathSolver()
	{
		License_SetString("2069810742&Courtesy_License&&&USR&2013&14_12_2011&1000&PATH&GEN&31_12_2013&0_0_0&0&0_0");
		e.error_data = 0;
		e.warning = MyWarning;
		e.error = MyError;
		Error_SetInterface(&e);
	}

	virtual bool solveMLCP(const btMatrixXu &A, const btVectorXu &b, btVectorXu &x, const btVectorXu &lo, const btVectorXu &hi, const btAlignedObjectArray<int> &limitDependency, int numIterations, bool useSparsity = true)
	{
		MCP_Termination status;

		int numVariables = b.rows();
		if (0 == numVariables)
			return true;

		
		btAlignedObjectArray<double> values;
		btAlignedObjectArray<int> rowIndices;
		btAlignedObjectArray<int> colIndices;

		for (int i = 0; i < A.rows(); i++)
		{
			for (int j = 0; j < A.cols(); j++)
			{
				if (A(i, j) != 0.f)
				{
					
					rowIndices.push_back(i + 1);
					colIndices.push_back(j + 1);
					values.push_back(A(i, j));
				}
			}
		}
		int numNonZero = rowIndices.size();
		btAlignedObjectArray<double> zResult;
		zResult.resize(numVariables);
		btAlignedObjectArray<double> rhs;
		btAlignedObjectArray<double> upperBounds;
		btAlignedObjectArray<double> lowerBounds;
		for (int i = 0; i < numVariables; i++)
		{
			upperBounds.push_back(hi[i]);
			lowerBounds.push_back(lo[i]);
			rhs.push_back(-b[i]);
		}

		SimpleLCP(numVariables, numNonZero, &rowIndices[0], &colIndices[0], &values[0], &rhs[0], &lowerBounds[0], &upperBounds[0], &status, &zResult[0]);

		if (status != MCP_Solved)
		{
			static const char *gReturnMsgs[] = {
				"Invalid return",
				"MCP_Solved: The problem was solved",
				"MCP_NoProgress: A stationary point was found",
				"MCP_MajorIterationLimit: Major iteration limit met",
				"MCP_MinorIterationLimit: Cumulative minor iteration limit met",
				"MCP_TimeLimit: Ran out of time",
				"MCP_UserInterrupt: Control-C, typically",
				"MCP_BoundError: Problem has a bound error",
				"MCP_DomainError: Could not find starting point",
				"MCP_Infeasible: Problem has no solution",
				"MCP_Error: An error occurred within the code",
				"MCP_LicenseError: License could not be found",
				"MCP_OK"};

			printf("ERROR: The PATH MCP solver failed: %s\n", gReturnMsgs[(unsigned int)status]);  
			printf("using Projected Gauss Seidel fallback\n");

			return false;
		}
		else
		{
			for (int i = 0; i < numVariables; i++)
			{
				x[i] = zResult[i];
				
				if (x[i] != zResult[i])
					return false;
			}
			return true;
		}
	}
};

#endif  

#endif  
