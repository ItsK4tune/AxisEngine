







#include "btLemkeAlgorithm.h"

#undef BT_DEBUG_OSTREAM
#ifdef BT_DEBUG_OSTREAM
using namespace std;
#endif  

btScalar btMachEps()
{
	static bool calculated = false;
	static btScalar machEps = btScalar(1.);
	if (!calculated)
	{
		do
		{
			machEps /= btScalar(2.0);
			
			
		} while ((btScalar)(1.0 + (machEps / btScalar(2.0))) != btScalar(1.0));
		
		calculated = true;
	}
	return machEps;
}

btScalar btEpsRoot()
{
	static btScalar epsroot = 0.;
	static bool alreadyCalculated = false;

	if (!alreadyCalculated)
	{
		epsroot = btSqrt(btMachEps());
		alreadyCalculated = true;
	}
	return epsroot;
}

btVectorXu btLemkeAlgorithm::solve(unsigned int maxloops )
{
	steps = 0;

	int dim = m_q.size();
#ifdef BT_DEBUG_OSTREAM
	if (DEBUGLEVEL >= 1)
	{
		cout << "Dimension = " << dim << endl;
	}
#endif  

	btVectorXu solutionVector(2 * dim);
	solutionVector.setZero();

	

	btMatrixXu ident(dim, dim);
	ident.setIdentity();
#ifdef BT_DEBUG_OSTREAM
	cout << m_M << std::endl;
#endif

	btMatrixXu mNeg = m_M.negative();

	btMatrixXu A(dim, 2 * dim + 2);
	
	A.setSubMatrix(0, 0, dim - 1, dim - 1, ident);
	A.setSubMatrix(0, dim, dim - 1, 2 * dim - 1, mNeg);
	A.setSubMatrix(0, 2 * dim, dim - 1, 2 * dim, -1.f);
	A.setSubMatrix(0, 2 * dim + 1, dim - 1, 2 * dim + 1, m_q);

#ifdef BT_DEBUG_OSTREAM
	cout << A << std::endl;
#endif  

	
	

	btAlignedObjectArray<int> basis;
	
	for (int i = 0; i < dim; i++)
		basis.push_back(i);

	int pivotRowIndex = -1;
	btScalar minValue = 1e30f;
	bool greaterZero = true;
	for (int i = 0; i < dim; i++)
	{
		btScalar v = A(i, 2 * dim + 1);
		if (v < minValue)
		{
			minValue = v;
			pivotRowIndex = i;
		}
		if (v < 0)
			greaterZero = false;
	}

	
	int z0Row = pivotRowIndex;    
	int pivotColIndex = 2 * dim;  

#ifdef BT_DEBUG_OSTREAM
	if (DEBUGLEVEL >= 3)
	{
		
		cout << "pivotRowIndex " << pivotRowIndex << endl;
		cout << "pivotColIndex " << pivotColIndex << endl;
		cout << "Basis: ";
		for (int i = 0; i < basis.size(); i++)
			cout << basis[i] << " ";
		cout << endl;
	}
#endif  

	if (!greaterZero)
	{
		if (maxloops == 0)
		{
			maxloops = 100;
			
		}

		
		for (steps = 0; steps < maxloops; steps++)
		{
			GaussJordanEliminationStep(A, pivotRowIndex, pivotColIndex, basis);
#ifdef BT_DEBUG_OSTREAM
			if (DEBUGLEVEL >= 3)
			{
				
				cout << "pivotRowIndex " << pivotRowIndex << endl;
				cout << "pivotColIndex " << pivotColIndex << endl;
				cout << "Basis: ";
				for (int i = 0; i < basis.size(); i++)
					cout << basis[i] << " ";
				cout << endl;
			}
#endif  

			int pivotColIndexOld = pivotColIndex;

			
			if (basis[pivotRowIndex] < dim)  
				pivotColIndex = basis[pivotRowIndex] + dim;
			else
				
				pivotColIndex = basis[pivotRowIndex] - dim;

			
			basis[pivotRowIndex] = pivotColIndexOld;
			bool isRayTermination = false;
			pivotRowIndex = findLexicographicMinimum(A, pivotColIndex, z0Row, isRayTermination);
			if (isRayTermination)
			{
				break; 
			}
			if (z0Row == pivotRowIndex)
			{  
				GaussJordanEliminationStep(A, pivotRowIndex, pivotColIndex, basis);
				basis[pivotRowIndex] = pivotColIndex;  
				break;
			}
		}
#ifdef BT_DEBUG_OSTREAM
		if (DEBUGLEVEL >= 1)
		{
			cout << "Number of loops: " << steps << endl;
			cout << "Number of maximal loops: " << maxloops << endl;
		}
#endif  

		if (!validBasis(basis))
		{
			info = -1;
#ifdef BT_DEBUG_OSTREAM
			if (DEBUGLEVEL >= 1)
				cerr << "Lemke-Algorithm ended with Ray-Termination (no valid solution)." << endl;
#endif  

			return solutionVector;
		}
	}
#ifdef BT_DEBUG_OSTREAM
	if (DEBUGLEVEL >= 2)
	{
		
		cout << "pivotRowIndex " << pivotRowIndex << endl;
		cout << "pivotColIndex " << pivotColIndex << endl;
	}
#endif  

	for (int i = 0; i < basis.size(); i++)
	{
		solutionVector[basis[i]] = A(i, 2 * dim + 1);  
	}

	info = 0;

	return solutionVector;
}

int btLemkeAlgorithm::findLexicographicMinimum(const btMatrixXu& A, const int& pivotColIndex, const int& z0Row, bool& isRayTermination)
{
	isRayTermination = false;
	btAlignedObjectArray<int> activeRows;

        bool firstRow = true;
	btScalar currentMin = 0.0;

	int dim = A.rows();

	for (int row = 0; row < dim; row++)
	{
		const btScalar denom = A(row, pivotColIndex);

		if (denom > btMachEps())
		{
			const btScalar q = A(row, dim + dim + 1) / denom;
			if (firstRow)
			{
				currentMin = q;
				activeRows.push_back(row);
				firstRow = false;
			}
			else if (fabs(currentMin - q) < btMachEps())
			{
				activeRows.push_back(row);
			}
			else if (currentMin > q)
			{
				currentMin = q;
				activeRows.clear();
				activeRows.push_back(row);
			}
		}
	}

	if (activeRows.size() == 0)
	{
		isRayTermination = true;
		return 0;
	}
	else if (activeRows.size() == 1)
	{
		return activeRows[0];
	}

	
	for (int i = 0; i < activeRows.size(); i++)
	{
		if (activeRows[i] == z0Row)
		{
			return z0Row;
		}
	}

	
	for (int col = 0; col < dim ; col++)
	{
		btAlignedObjectArray<int> activeRowsCopy(activeRows);
		activeRows.clear();
		firstRow = true;
		for (int i = 0; i<activeRowsCopy.size();i++)
		{
			const int row = activeRowsCopy[i];

			
			const btScalar denom = A(row, pivotColIndex);
			const btScalar ratio = A(row, col) / denom;
			if (firstRow)
			{
				currentMin = ratio;
				activeRows.push_back(row);
				firstRow = false;
			}
			else if (fabs(currentMin - ratio) < btMachEps())
			{
				activeRows.push_back(row);
			}
			else if (currentMin > ratio)
			{
				currentMin = ratio;
				activeRows.clear();
				activeRows.push_back(row);
			}
		}

		if (activeRows.size() == 1)
		{
			return activeRows[0];
		}
	}
	
	isRayTermination = true;
	return 0;
}

void btLemkeAlgorithm::GaussJordanEliminationStep(btMatrixXu& A, int pivotRowIndex, int pivotColumnIndex, const btAlignedObjectArray<int>& basis)
{
	btScalar a = -1 / A(pivotRowIndex, pivotColumnIndex);
#ifdef BT_DEBUG_OSTREAM
	cout << A << std::endl;
#endif

	for (int i = 0; i < A.rows(); i++)
	{
		if (i != pivotRowIndex)
		{
			for (int j = 0; j < A.cols(); j++)
			{
				if (j != pivotColumnIndex)
				{
					btScalar v = A(i, j);
					v += A(pivotRowIndex, j) * A(i, pivotColumnIndex) * a;
					A.setElem(i, j, v);
				}
			}
		}
	}

#ifdef BT_DEBUG_OSTREAM
	cout << A << std::endl;
#endif  
	for (int i = 0; i < A.cols(); i++)
	{
		A.mulElem(pivotRowIndex, i, -a);
	}
#ifdef BT_DEBUG_OSTREAM
	cout << A << std::endl;
#endif  

	for (int i = 0; i < A.rows(); i++)
	{
		if (i != pivotRowIndex)
		{
			A.setElem(i, pivotColumnIndex, 0);
		}
	}
#ifdef BT_DEBUG_OSTREAM
	cout << A << std::endl;
#endif  
}

bool btLemkeAlgorithm::greaterZero(const btVectorXu& vector)
{
	bool isGreater = true;
	for (int i = 0; i < vector.size(); i++)
	{
		if (vector[i] < 0)
		{
			isGreater = false;
			break;
		}
	}

	return isGreater;
}

bool btLemkeAlgorithm::validBasis(const btAlignedObjectArray<int>& basis)
{
	bool isValid = true;
	for (int i = 0; i < basis.size(); i++)
	{
		if (basis[i] >= basis.size() * 2)
		{  
			isValid = false;
			break;
		}
	}

	return isValid;
}
