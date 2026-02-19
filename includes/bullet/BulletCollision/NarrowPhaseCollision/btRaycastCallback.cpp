



#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h"
#include "BulletCollision/NarrowPhaseCollision/btGjkConvexCast.h"
#include "BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h"
#include "BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h"
#include "btRaycastCallback.h"

btTriangleRaycastCallback::btTriangleRaycastCallback(const btVector3& from, const btVector3& to, unsigned int flags)
	: m_from(from),
	  m_to(to),
	  
	  m_flags(flags),
	  m_hitFraction(btScalar(1.))
{
}

void btTriangleRaycastCallback::processTriangle(btVector3* triangle, int partId, int triangleIndex)
{
	const btVector3& vert0 = triangle[0];
	const btVector3& vert1 = triangle[1];
	const btVector3& vert2 = triangle[2];

	btVector3 v10;
	v10 = vert1 - vert0;
	btVector3 v20;
	v20 = vert2 - vert0;

	btVector3 triangleNormal;
	triangleNormal = v10.cross(v20);

	const btScalar dist = vert0.dot(triangleNormal);
	btScalar dist_a = triangleNormal.dot(m_from);
	dist_a -= dist;
	btScalar dist_b = triangleNormal.dot(m_to);
	dist_b -= dist;

	if (dist_a * dist_b >= btScalar(0.0))
	{
		return;  
	}

	if (((m_flags & kF_FilterBackfaces) != 0) && (dist_a <= btScalar(0.0)))
	{
		
		return;
	}

	const btScalar proj_length = dist_a - dist_b;
	const btScalar distance = (dist_a) / (proj_length);
	
	
	
	

	if (distance < m_hitFraction)
	{
		btScalar edge_tolerance = triangleNormal.length2();
		edge_tolerance *= btScalar(-0.0001);
		btVector3 point;
		point.setInterpolate3(m_from, m_to, distance);
		{
			btVector3 v0p;
			v0p = vert0 - point;
			btVector3 v1p;
			v1p = vert1 - point;
			btVector3 cp0;
			cp0 = v0p.cross(v1p);

			if ((btScalar)(cp0.dot(triangleNormal)) >= edge_tolerance)
			{
				btVector3 v2p;
				v2p = vert2 - point;
				btVector3 cp1;
				cp1 = v1p.cross(v2p);
				if ((btScalar)(cp1.dot(triangleNormal)) >= edge_tolerance)
				{
					btVector3 cp2;
					cp2 = v2p.cross(v0p);

					if ((btScalar)(cp2.dot(triangleNormal)) >= edge_tolerance)
					{
						
						
						triangleNormal.normalize();

						
						if (((m_flags & kF_KeepUnflippedNormal) == 0) && (dist_a <= btScalar(0.0)))
						{
							m_hitFraction = reportHit(-triangleNormal, distance, partId, triangleIndex);
						}
						else
						{
							m_hitFraction = reportHit(triangleNormal, distance, partId, triangleIndex);
						}
					}
				}
			}
		}
	}
}

btTriangleConvexcastCallback::btTriangleConvexcastCallback(const btConvexShape* convexShape, const btTransform& convexShapeFrom, const btTransform& convexShapeTo, const btTransform& triangleToWorld, const btScalar triangleCollisionMargin)
{
	m_convexShape = convexShape;
	m_convexShapeFrom = convexShapeFrom;
	m_convexShapeTo = convexShapeTo;
	m_triangleToWorld = triangleToWorld;
	m_hitFraction = 1.0f;
	m_triangleCollisionMargin = triangleCollisionMargin;
	m_allowedPenetration = 0.f;
}

void btTriangleConvexcastCallback::processTriangle(btVector3* triangle, int partId, int triangleIndex)
{
	btTriangleShape triangleShape(triangle[0], triangle[1], triangle[2]);
	triangleShape.setMargin(m_triangleCollisionMargin);

	btVoronoiSimplexSolver simplexSolver;
	btGjkEpaPenetrationDepthSolver gjkEpaPenetrationSolver;



#ifdef USE_SUBSIMPLEX_CONVEX_CAST
	btSubsimplexConvexCast convexCaster(m_convexShape, &triangleShape, &simplexSolver);
#else
	
	btContinuousConvexCollision convexCaster(m_convexShape, &triangleShape, &simplexSolver, &gjkEpaPenetrationSolver);
#endif  

	btConvexCast::CastResult castResult;
	castResult.m_fraction = btScalar(1.);
	castResult.m_allowedPenetration = m_allowedPenetration;
	if (convexCaster.calcTimeOfImpact(m_convexShapeFrom, m_convexShapeTo, m_triangleToWorld, m_triangleToWorld, castResult))
	{
		
		if (castResult.m_normal.length2() > btScalar(0.0001))
		{
			if (castResult.m_fraction < m_hitFraction)
			{
				
				
				castResult.m_normal.normalize();

				reportHit(castResult.m_normal,
						  castResult.m_hitPoint,
						  castResult.m_fraction,
						  partId,
						  triangleIndex);
			}
		}
	}
}
