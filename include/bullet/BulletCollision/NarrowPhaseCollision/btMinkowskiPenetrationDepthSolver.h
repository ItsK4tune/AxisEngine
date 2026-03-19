

#ifndef BT_MINKOWSKI_PENETRATION_DEPTH_SOLVER_H
#define BT_MINKOWSKI_PENETRATION_DEPTH_SOLVER_H

#include "btConvexPenetrationDepthSolver.h"



class btMinkowskiPenetrationDepthSolver : public btConvexPenetrationDepthSolver
{
protected:
	static btVector3* getPenetrationDirections();

public:
	virtual bool calcPenDepth(btSimplexSolverInterface& simplexSolver,
							  const btConvexShape* convexA, const btConvexShape* convexB,
							  const btTransform& transA, const btTransform& transB,
							  btVector3& v, btVector3& pa, btVector3& pb,
							  class btIDebugDraw* debugDraw);
};

#endif  
