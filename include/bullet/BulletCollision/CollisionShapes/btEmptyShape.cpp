

#include "btEmptyShape.h"

#include "btCollisionShape.h"

btEmptyShape::btEmptyShape() : btConcaveShape()
{
	m_shapeType = EMPTY_SHAPE_PROXYTYPE;
}

btEmptyShape::~btEmptyShape()
{
}


void btEmptyShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	btVector3 margin(getMargin(), getMargin(), getMargin());

	aabbMin = t.getOrigin() - margin;

	aabbMax = t.getOrigin() + margin;
}

void btEmptyShape::calculateLocalInertia(btScalar, btVector3&) const
{
	btAssert(0);
}
