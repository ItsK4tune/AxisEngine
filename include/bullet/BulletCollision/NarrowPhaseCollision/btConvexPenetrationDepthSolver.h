

#ifndef BT_CONVEX_PENETRATION_DEPTH_H
#define BT_CONVEX_PENETRATION_DEPTH_H

class btVector3;
#include "btSimplexSolverInterface.h"
class btConvexShape;
class btTransform;


class btConvexPenetrationDepthSolver
{
public:
	virtual ~btConvexPenetrationDepthSolver(){};
	virtual bool calcPenDepth(btSimplexSolverInterface& simplexSolver,
							  const btConvexShape* convexA, const btConvexShape* convexB,
							  const btTransform& transA, const btTransform& transB,
							  btVector3& v, btVector3& pa, btVector3& pb,
							  class btIDebugDraw* debugDraw) = 0;
};
#endif  
