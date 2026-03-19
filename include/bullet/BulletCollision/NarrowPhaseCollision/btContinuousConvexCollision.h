

#ifndef BT_CONTINUOUS_COLLISION_CONVEX_CAST_H
#define BT_CONTINUOUS_COLLISION_CONVEX_CAST_H

#include "btConvexCast.h"
#include "btSimplexSolverInterface.h"
class btConvexPenetrationDepthSolver;
class btConvexShape;
class btStaticPlaneShape;





class btContinuousConvexCollision : public btConvexCast
{
	btSimplexSolverInterface* m_simplexSolver;
	btConvexPenetrationDepthSolver* m_penetrationDepthSolver;
	const btConvexShape* m_convexA;
	
	const btConvexShape* m_convexB1;
	const btStaticPlaneShape* m_planeShape;

	void computeClosestPoints(const btTransform& transA, const btTransform& transB, struct btPointCollector& pointCollector);

public:
	btContinuousConvexCollision(const btConvexShape* shapeA, const btConvexShape* shapeB, btSimplexSolverInterface* simplexSolver, btConvexPenetrationDepthSolver* penetrationDepthSolver);

	btContinuousConvexCollision(const btConvexShape* shapeA, const btStaticPlaneShape* plane);

	virtual bool calcTimeOfImpact(
		const btTransform& fromA,
		const btTransform& toA,
		const btTransform& fromB,
		const btTransform& toB,
		CastResult& result);
};

#endif  
