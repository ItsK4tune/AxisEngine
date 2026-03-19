
#include "btBoxShape.h"

btBoxShape::btBoxShape(const btVector3& boxHalfExtents)
	: btPolyhedralConvexShape()
{
	m_shapeType = BOX_SHAPE_PROXYTYPE;

	btVector3 margin(getMargin(), getMargin(), getMargin());
	m_implicitShapeDimensions = (boxHalfExtents * m_localScaling) - margin;

	setSafeMargin(boxHalfExtents);
};

void btBoxShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	btTransformAabb(getHalfExtentsWithoutMargin(), getMargin(), t, aabbMin, aabbMax);
}

void btBoxShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	
	btVector3 halfExtents = getHalfExtentsWithMargin();

	btScalar lx = btScalar(2.) * (halfExtents.x());
	btScalar ly = btScalar(2.) * (halfExtents.y());
	btScalar lz = btScalar(2.) * (halfExtents.z());

	inertia.setValue(mass / (btScalar(12.0)) * (ly * ly + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + lz * lz),
					 mass / (btScalar(12.0)) * (lx * lx + ly * ly));
}
