

#include "btSubSimplexConvexCast.h"
#include "BulletCollision/CollisionShapes/btConvexShape.h"

#include "BulletCollision/CollisionShapes/btMinkowskiSumShape.h"
#include "BulletCollision/NarrowPhaseCollision/btSimplexSolverInterface.h"
#include "btPointCollector.h"
#include "LinearMath/btTransformUtil.h"

btSubsimplexConvexCast::btSubsimplexConvexCast(const btConvexShape* convexA, const btConvexShape* convexB, btSimplexSolverInterface* simplexSolver)
	: m_simplexSolver(simplexSolver),
	  m_convexA(convexA),
	  m_convexB(convexB)
{
}


bool btSubsimplexConvexCast::calcTimeOfImpact(
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

	btScalar lambda = btScalar(0.);

	btTransform interpolatedTransA = fromA;
	btTransform interpolatedTransB = fromB;

	
	btVector3 r = (linVelA - linVelB);
	btVector3 v;

	btVector3 supVertexA = fromA(m_convexA->localGetSupportingVertex(-r * fromA.getBasis()));
	btVector3 supVertexB = fromB(m_convexB->localGetSupportingVertex(r * fromB.getBasis()));
	v = supVertexA - supVertexB;
	int maxIter = result.m_subSimplexCastMaxIterations;

	btVector3 n;
	n.setValue(btScalar(0.), btScalar(0.), btScalar(0.));

	btVector3 c;

	btScalar dist2 = v.length2();



	btVector3 w, p;
	btScalar VdotR;

	while ((dist2 > result.m_subSimplexCastEpsilon) && maxIter--)
	{
		supVertexA = interpolatedTransA(m_convexA->localGetSupportingVertex(-v * interpolatedTransA.getBasis()));
		supVertexB = interpolatedTransB(m_convexB->localGetSupportingVertex(v * interpolatedTransB.getBasis()));
		w = supVertexA - supVertexB;

		btScalar VdotW = v.dot(w);

		if (lambda > btScalar(1.0))
		{
			return false;
		}

		if (VdotW > btScalar(0.))
		{
			VdotR = v.dot(r);

			if (VdotR >= -(SIMD_EPSILON * SIMD_EPSILON))
				return false;
			else
			{
				lambda = lambda - VdotW / VdotR;
				
				
				interpolatedTransA.getOrigin().setInterpolate3(fromA.getOrigin(), toA.getOrigin(), lambda);
				interpolatedTransB.getOrigin().setInterpolate3(fromB.getOrigin(), toB.getOrigin(), lambda);
				
				
				w = supVertexA - supVertexB;

				n = v;
			}
		}
		
		if (!m_simplexSolver->inSimplex(w))
			m_simplexSolver->addVertex(w, supVertexA, supVertexB);

		if (m_simplexSolver->closest(v))
		{
			dist2 = v.length2();

			
			
			
			
			
		}
		else
		{
			dist2 = btScalar(0.);
		}
	}

	
	

	

	result.m_fraction = lambda;
	if (n.length2() >= (SIMD_EPSILON * SIMD_EPSILON))
		result.m_normal = n.normalized();
	else
		result.m_normal = btVector3(btScalar(0.0), btScalar(0.0), btScalar(0.0));

	
	if (result.m_normal.dot(r) >= -result.m_allowedPenetration)
		return false;

	btVector3 hitA, hitB;
	m_simplexSolver->compute_points(hitA, hitB);
	result.m_hitPoint = hitB;
	return true;
}
