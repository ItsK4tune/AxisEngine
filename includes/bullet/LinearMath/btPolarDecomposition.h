#ifndef POLARDECOMPOSITION_H
#define POLARDECOMPOSITION_H

#include "btMatrix3x3.h"


class btPolarDecomposition
{
public:
	
	btPolarDecomposition(btScalar tolerance = btScalar(0.0001),
						 unsigned int maxIterations = 16);

	
	unsigned int decompose(const btMatrix3x3& a, btMatrix3x3& u, btMatrix3x3& h) const;

	
	unsigned int maxIterations() const;

private:
	btScalar m_tolerance;
	unsigned int m_maxIterations;
};


unsigned int polarDecompose(const btMatrix3x3& a, btMatrix3x3& u, btMatrix3x3& h);

#endif  
