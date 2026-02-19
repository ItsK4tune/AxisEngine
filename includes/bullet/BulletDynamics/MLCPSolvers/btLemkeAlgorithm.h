







#ifndef BT_NUMERICS_LEMKE_ALGORITHM_H_
#define BT_NUMERICS_LEMKE_ALGORITHM_H_

#include "LinearMath/btMatrixX.h"

#include <vector>  

class btLemkeAlgorithm
{
public:
	btLemkeAlgorithm(const btMatrixXu& M_, const btVectorXu& q_, const int& DEBUGLEVEL_ = 0) : DEBUGLEVEL(DEBUGLEVEL_)
	{
		setSystem(M_, q_);
	}

	
	
	int getInfo()
	{
		return info;
	}

	
	int getSteps(void)
	{
		return steps;
	}

	
	void setSystem(const btMatrixXu& M_, const btVectorXu& q_)
	{
		m_M = M_;
		m_q = q_;
	}
	

	
	btVectorXu solve(unsigned int maxloops = 0);

	virtual ~btLemkeAlgorithm()
	{
	}

protected:
	int findLexicographicMinimum(const btMatrixXu& A, const int& pivotColIndex, const int& z0Row, bool& isRayTermination);
	void GaussJordanEliminationStep(btMatrixXu& A, int pivotRowIndex, int pivotColumnIndex, const btAlignedObjectArray<int>& basis);
	bool greaterZero(const btVectorXu& vector);
	bool validBasis(const btAlignedObjectArray<int>& basis);

	btMatrixXu m_M;
	btVectorXu m_q;

	
	unsigned int steps;

	
	int DEBUGLEVEL;

	
	int info;
};

#endif 
