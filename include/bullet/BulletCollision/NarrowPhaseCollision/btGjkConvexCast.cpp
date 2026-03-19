

#include "btGjkConvexCast.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "btGjkPairDetector.h"
#include "btPointCollector.h"
#include "LinearMath/btTransformUtil.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define MAX_ITERATIONS 64
#else
#define MAX_ITERATIONS 32
#endif

btGjkConvexCast::btGjkConvexCast(const btConvexShape* convexA, const btConvexShape* convexB, btSimplexSolverInterface* simplexSolver)
	: m_simplexSolver(simplexSolver),
	  m_convexA(convexA),
	  m_convexB(convexB)
{
}

bool btGjkConvexCast::calcTimeOfImpact(
	const btTransform& fromA,
	const btTransform& toA,
	const btTransform& fromB,
	const btTransform& toB,
	CastResult& result)
{
	m_simplexSolver->reset();

	
	
	btVector3 linVelA, linVelB;
	linVelA = toA.getOrigin() - fromA.getOrigin();
	linVelB = toB.getOrigin() - fromB.getOrigin();

	btScalar radius = btScalar(0.001);
	btScalar lambda = btScalar(0.);
	btVector3 v(1, 0, 0);

	int maxIter = MAX_ITERATIONS;

	btVector3 n;
	n.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
	bool hasResult = false;
	btVector3 c;
	btVector3 r = (linVelA - linVelB);

	btScalar lastLambda = lambda;
	

	int numIter = 0;
	

	btTransform identityTrans;
	identityTrans.setIdentity();

	

	btPointCollector pointCollector;

	btGjkPairDetector gjk(m_convexA, m_convexB, m_simplexSolver, 0);  
	btGjkPairDetector::ClosestPointInput input;

	
	

	input.m_transformA = fromA;
	input.m_transformB = fromB;
	gjk.getClosestPoints(input, pointCollector, 0);

	hasResult = pointCollector.m_hasResult;
	c = pointCollector.m_pointInWorld;

	if (hasResult)
	{
		btScalar dist;
		dist = pointCollector.m_distance;
		n = pointCollector.m_normalOnBInWorld;

		
		while (dist > radius)
		{
			numIter++;
			if (numIter > maxIter)
			{
				return false;  
			}
			btScalar dLambda = btScalar(0.);

			btScalar projectedLinearVelocity = r.dot(n);

			dLambda = dist / (projectedLinearVelocity);

			lambda = lambda - dLambda;

			if (lambda > btScalar(1.))
				return false;

			if (lambda < btScalar(0.))
				return false;

			
			if (lambda <= lastLambda)
			{
				return false;
				
				break;
			}
			lastLambda = lambda;

			
			result.DebugDraw(lambda);
			input.m_transformA.getOrigin().setInterpolate3(fromA.getOrigin(), toA.getOrigin(), lambda);
			input.m_transformB.getOrigin().setInterpolate3(fromB.getOrigin(), toB.getOrigin(), lambda);

			gjk.getClosestPoints(input, pointCollector, 0);
			if (pointCollector.m_hasResult)
			{
				if (pointCollector.m_distance < btScalar(0.))
				{
					result.m_fraction = lastLambda;
					n = pointCollector.m_normalOnBInWorld;
					result.m_normal = n;
					result.m_hitPoint = pointCollector.m_pointInWorld;
					return true;
				}
				c = pointCollector.m_pointInWorld;
				n = pointCollector.m_normalOnBInWorld;
				dist = pointCollector.m_distance;
			}
			else
			{
				
				return false;
			}
		}

		
		
		if (n.dot(r) >= -result.m_allowedPenetration)
			return false;

		result.m_fraction = lambda;
		result.m_normal = n;
		result.m_hitPoint = c;
		return true;
	}

	return false;
}
