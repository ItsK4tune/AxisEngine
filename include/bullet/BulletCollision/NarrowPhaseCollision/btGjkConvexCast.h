

#ifndef BT_GJK_CONVEX_CAST_H
#define BT_GJK_CONVEX_CAST_H

#include "BulletCollision/CollisionShapes/btCollisionMargin.h"

#include "LinearMath/btVector3.h"
#include "btConvexCast.h"
class btConvexShape;
class btMinkowskiSumShape;
#include "btSimplexSolverInterface.h"


class btGjkConvexCast : public btConvexCast
{
	btSimplexSolverInterface* m_simplexSolver;
	const btConvexShape* m_convexA;
	const btConvexShape* m_convexB;

public:
	btGjkConvexCast(const btConvexShape* convexA, const btConvexShape* convexB, btSimplexSolverInterface* simplexSolver);

	
	virtual bool calcTimeOfImpact(
		const btTransform& fromA,
		const btTransform& toA,
		const btTransform& fromB,
		const btTransform& toB,
		CastResult& result);
};

#endif  
