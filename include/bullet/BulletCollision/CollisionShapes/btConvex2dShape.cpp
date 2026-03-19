

#include "btConvex2dShape.h"

btConvex2dShape::btConvex2dShape(btConvexShape* convexChildShape) : btConvexShape(), m_childConvexShape(convexChildShape)
{
	m_shapeType = CONVEX_2D_SHAPE_PROXYTYPE;
}

btConvex2dShape::~btConvex2dShape()
{
}

btVector3 btConvex2dShape::localGetSupportingVertexWithoutMargin(const btVector3& vec) const
{
	return m_childConvexShape->localGetSupportingVertexWithoutMargin(vec);
}

void btConvex2dShape::batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const
{
	m_childConvexShape->batchedUnitVectorGetSupportingVertexWithoutMargin(vectors, supportVerticesOut, numVectors);
}

btVector3 btConvex2dShape::localGetSupportingVertex(const btVector3& vec) const
{
	return m_childConvexShape->localGetSupportingVertex(vec);
}

void btConvex2dShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	
	m_childConvexShape->calculateLocalInertia(mass, inertia);
}


void btConvex2dShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	m_childConvexShape->getAabb(t, aabbMin, aabbMax);
}

void btConvex2dShape::getAabbSlow(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	m_childConvexShape->getAabbSlow(t, aabbMin, aabbMax);
}

void btConvex2dShape::setLocalScaling(const btVector3& scaling)
{
	m_childConvexShape->setLocalScaling(scaling);
}

const btVector3& btConvex2dShape::getLocalScaling() const
{
	return m_childConvexShape->getLocalScaling();
}

void btConvex2dShape::setMargin(btScalar margin)
{
	m_childConvexShape->setMargin(margin);
}
btScalar btConvex2dShape::getMargin() const
{
	return m_childConvexShape->getMargin();
}

int btConvex2dShape::getNumPreferredPenetrationDirections() const
{
	return m_childConvexShape->getNumPreferredPenetrationDirections();
}

void btConvex2dShape::getPreferredPenetrationDirection(int index, btVector3& penetrationVector) const
{
	m_childConvexShape->getPreferredPenetrationDirection(index, penetrationVector);
}
