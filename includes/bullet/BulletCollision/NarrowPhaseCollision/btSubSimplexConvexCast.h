

#ifndef BT_SUBSIMPLEX_CONVEX_CAST_H
#define BT_SUBSIMPLEX_CONVEX_CAST_H

#include "btConvexCast.h"
#include "btSimplexSolverInterface.h"
class btConvexShape;





class btSubsimplexConvexCast : public btConvexCast
{
	btSimplexSolverInterface* m_simplexSolver;
	const btConvexShape* m_convexA;
	const btConvexShape* m_convexB;

public:
	btSubsimplexConvexCast(const btConvexShape* shapeA, const btConvexShape* shapeB, btSimplexSolverInterface* simplexSolver);

	
	
	
	virtual bool calcTimeOfImpact(
		const btTransform& fromA,
		const btTransform& toA,
		const btTransform& fromB,
		const btTransform& toB,
		CastResult& result);
};

#endif  
