

#include "LinearMath/btScalar.h"
#include "SphereTriangleDetector.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"

SphereTriangleDetector::SphereTriangleDetector(btSphereShape* sphere, btTriangleShape* triangle, btScalar contactBreakingThreshold)
	: m_sphere(sphere),
	  m_triangle(triangle),
	  m_contactBreakingThreshold(contactBreakingThreshold)
{
}

void SphereTriangleDetector::getClosestPoints(const ClosestPointInput& input, Result& output, class btIDebugDraw* debugDraw, bool swapResults)
{
	(void)debugDraw;
	const btTransform& transformA = input.m_transformA;
	const btTransform& transformB = input.m_transformB;

	btVector3 point, normal;
	btScalar timeOfImpact = btScalar(1.);
	btScalar depth = btScalar(0.);
	
	
	btTransform sphereInTr = transformB.inverseTimes(transformA);

	if (collide(sphereInTr.getOrigin(), point, normal, depth, timeOfImpact, m_contactBreakingThreshold))
	{
		if (swapResults)
		{
			btVector3 normalOnB = transformB.getBasis() * normal;
			btVector3 normalOnA = -normalOnB;
			btVector3 pointOnA = transformB * point + normalOnB * depth;
			output.addContactPoint(normalOnA, pointOnA, depth);
		}
		else
		{
			output.addContactPoint(transformB.getBasis() * normal, transformB * point, depth);
		}
	}
}



btScalar SegmentSqrDistance(const btVector3& from, const btVector3& to, const btVector3& p, btVector3& nearest);

btScalar SegmentSqrDistance(const btVector3& from, const btVector3& to, const btVector3& p, btVector3& nearest)
{
	btVector3 diff = p - from;
	btVector3 v = to - from;
	btScalar t = v.dot(diff);

	if (t > 0)
	{
		btScalar dotVV = v.dot(v);
		if (t < dotVV)
		{
			t /= dotVV;
			diff -= t * v;
		}
		else
		{
			t = 1;
			diff -= v;
		}
	}
	else
		t = 0;

	nearest = from + t * v;
	return diff.dot(diff);
}

bool SphereTriangleDetector::facecontains(const btVector3& p, const btVector3* vertices, btVector3& normal)
{
	btVector3 lp(p);
	btVector3 lnormal(normal);

	return pointInTriangle(vertices, lnormal, &lp);
}

bool SphereTriangleDetector::collide(const btVector3& sphereCenter, btVector3& point, btVector3& resultNormal, btScalar& depth, btScalar& timeOfImpact, btScalar contactBreakingThreshold)
{
	const btVector3* vertices = &m_triangle->getVertexPtr(0);

	btScalar radius = m_sphere->getRadius();
	btScalar radiusWithThreshold = radius + contactBreakingThreshold;

	btVector3 normal = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]);

	btScalar l2 = normal.length2();
	bool hasContact = false;
	btVector3 contactPoint;

	if (l2 >= SIMD_EPSILON * SIMD_EPSILON)
	{
		normal /= btSqrt(l2);

		btVector3 p1ToCentre = sphereCenter - vertices[0];
		btScalar distanceFromPlane = p1ToCentre.dot(normal);

		if (distanceFromPlane < btScalar(0.))
		{
			
			distanceFromPlane *= btScalar(-1.);
			normal *= btScalar(-1.);
		}

		bool isInsideContactPlane = distanceFromPlane < radiusWithThreshold;

		

		if (isInsideContactPlane)
		{
			if (facecontains(sphereCenter, vertices, normal))
			{
				
				hasContact = true;
				contactPoint = sphereCenter - normal * distanceFromPlane;
			}
			else
			{
				
				btScalar contactCapsuleRadiusSqr = radiusWithThreshold * radiusWithThreshold;
				btScalar minDistSqr = contactCapsuleRadiusSqr;
				btVector3 nearestOnEdge;
				for (int i = 0; i < m_triangle->getNumEdges(); i++)
				{
					btVector3 pa;
					btVector3 pb;

					m_triangle->getEdge(i, pa, pb);

					btScalar distanceSqr = SegmentSqrDistance(pa, pb, sphereCenter, nearestOnEdge);
					if (distanceSqr < minDistSqr)
					{
						
						minDistSqr = distanceSqr;
						hasContact = true;
						contactPoint = nearestOnEdge;
					}
				}
			}
		}
	}

	if (hasContact)
	{
		btVector3 contactToCentre = sphereCenter - contactPoint;
		btScalar distanceSqr = contactToCentre.length2();

		if (distanceSqr < radiusWithThreshold * radiusWithThreshold)
		{
			if (distanceSqr > SIMD_EPSILON)
			{
				btScalar distance = btSqrt(distanceSqr);
				resultNormal = contactToCentre;
				resultNormal.normalize();
				point = contactPoint;
				depth = -(radius - distance);
			}
			else
			{
				resultNormal = normal;
				point = contactPoint;
				depth = -radius;
			}
			return true;
		}
	}

	return false;
}

bool SphereTriangleDetector::pointInTriangle(const btVector3 vertices[], const btVector3& normal, btVector3* p)
{
	const btVector3* p1 = &vertices[0];
	const btVector3* p2 = &vertices[1];
	const btVector3* p3 = &vertices[2];

	btVector3 edge1(*p2 - *p1);
	btVector3 edge2(*p3 - *p2);
	btVector3 edge3(*p1 - *p3);

	btVector3 p1_to_p(*p - *p1);
	btVector3 p2_to_p(*p - *p2);
	btVector3 p3_to_p(*p - *p3);

	btVector3 edge1_normal(edge1.cross(normal));
	btVector3 edge2_normal(edge2.cross(normal));
	btVector3 edge3_normal(edge3.cross(normal));

	btScalar r1, r2, r3;
	r1 = edge1_normal.dot(p1_to_p);
	r2 = edge2_normal.dot(p2_to_p);
	r3 = edge3_normal.dot(p3_to_p);
	if ((r1 > 0 && r2 > 0 && r3 > 0) ||
		(r1 <= 0 && r2 <= 0 && r3 <= 0))
		return true;
	return false;
}
