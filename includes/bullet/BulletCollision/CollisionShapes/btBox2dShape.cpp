

#include "btBox2dShape.h"



void btBox2dShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	btTransformAabb(getHalfExtentsWithoutMargin(), getMargin(), t, aabbMin, aabbMax);
}

void btBox2dShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	
	btVector3 halfExtents = getHalfExtentsWithMargin();

	btScalar lx = btScalar(2.) * (halfExtents.x());
	btScalar ly = btScalar(2.) * (halfExtents.y());
	btScalar lz = btScalar(2.) * (halfExtents.z());

	inertia.setValue(mass / (btScalar(12.0)) * (ly * ly + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + ly * ly));
}
