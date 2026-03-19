

#include "btSphereShape.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"

#include "LinearMath/btQuaternion.h"

btVector3 btSphereShape::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	(void)vec;
	return btVector3(btScalar(0.), btScalar(0.), btScalar(0.));
}

void btSphereShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	(void)vectors;

	for (int i = 0; i < numVectors; i++)
	{
		supportVerticesOut[i].setValue(btScalar(0.), btScalar(0.), btScalar(0.));
	}
}

btVector3 btSphereShape::localGetSupportingVertex(const btVector3& vec) const
{
	btVector3 supVertex;
	supVertex = localGetSupportingVertexWithoutMargin(vec);

	btVector3 vecnorm = vec;
	if (vecnorm.length2() < (SIMD_EPSILON * SIMD_EPSILON))
	{
		vecnorm.setValue(btScalar(-1.), btScalar(-1.), btScalar(-1.));
	}
	vecnorm.normalize();
	supVertex += getMargin() * vecnorm;
	return supVertex;
}


void btSphereShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	const btVector3& center = t.getOrigin();
	btVector3 extent(getMargin(), getMargin(), getMargin());
	aabbMin = center - extent;
	aabbMax = center + extent;
}

void btSphereShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	btScalar elem = btScalar(0.4) * mass * getMargin() * getMargin();
	inertia.setValue(elem, elem, elem);
}
