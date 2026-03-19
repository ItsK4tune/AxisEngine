
#ifndef BT_GJP_EPA_PENETRATION_DEPTH_H
#define BT_GJP_EPA_PENETRATION_DEPTH_H

#include "btConvexPenetrationDepthSolver.h"



class btGjkEpaPenetrationDepthSolver : public btConvexPenetrationDepthSolver
{
public:
	btGjkEpaPenetrationDepthSolver()
	{
	}

	bool calcPenDepth(btSimplexSolverInterface& simplexSolver,
					  const btConvexShape* pConvexA, const btConvexShape* pConvexB,
					  const btTransform& transformA, const btTransform& transformB,
					  btVector3& v, btVector3& wWitnessOnA, btVector3& wWitnessOnB,
					  class btIDebugDraw* debugDraw);

private:
};

#endif  
