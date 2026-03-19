

#include "btStaticPlaneShape.h"

#include "LinearMath/btTransformUtil.h"

btStaticPlaneShape::btStaticPlaneShape(const btVector3& planeNormal, btScalar planeConstant)
	: btConcaveShape(), m_planeNormal(planeNormal.normalized()), m_planeConstant(planeConstant), m_localScaling(btScalar(1.), btScalar(1.), btScalar(1.))
{
	m_shapeType = STATIC_PLANE_PROXYTYPE;
	
}

btStaticPlaneShape::~btStaticPlaneShape()
{
}

void btStaticPlaneShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	(void)t;
	

	aabbMin.setValue(btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT), btScalar(-BT_LARGE_FLOAT));
	aabbMax.setValue(btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT), btScalar(BT_LARGE_FLOAT));
}

void btStaticPlaneShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
	btVector3 halfExtents = (aabbMax - aabbMin) * btScalar(0.5);
	btScalar radius = halfExtents.length();
	btVector3 center = (aabbMax + aabbMin) * btScalar(0.5);

	

	btVector3 tangentDir0, tangentDir1;

	
	btPlaneSpace1(m_planeNormal, tangentDir0, tangentDir1);

	btVector3 projectedCenter = center - (m_planeNormal.dot(center) - m_planeConstant) * m_planeNormal;

	btVector3 triangle[3];
	triangle[0] = projectedCenter + tangentDir0 * radius + tangentDir1 * radius;
	triangle[1] = projectedCenter + tangentDir0 * radius - tangentDir1 * radius;
	triangle[2] = projectedCenter - tangentDir0 * radius - tangentDir1 * radius;

	callback->processTriangle(triangle, 0, 0);

	triangle[0] = projectedCenter - tangentDir0 * radius - tangentDir1 * radius;
	triangle[1] = projectedCenter - tangentDir0 * radius + tangentDir1 * radius;
	triangle[2] = projectedCenter + tangentDir0 * radius + tangentDir1 * radius;

	callback->processTriangle(triangle, 0, 1);
}

void btStaticPlaneShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	(void)mass;

	

	inertia.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
}

void btStaticPlaneShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& btStaticPlaneShape::getLocalScaling() const
{
	return m_localScaling;
}
