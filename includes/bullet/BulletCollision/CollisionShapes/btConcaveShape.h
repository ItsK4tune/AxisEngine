

#ifndef BT_CONCAVE_SHAPE_H
#define BT_CONCAVE_SHAPE_H

#include "btCollisionShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  
#include "btTriangleCallback.h"



typedef enum PHY_ScalarType
{
	PHY_FLOAT,
	PHY_DOUBLE,
	PHY_INTEGER,
	PHY_SHORT,
	PHY_FIXEDPOINT88,
	PHY_UCHAR
} PHY_ScalarType;



ATTRIBUTE_ALIGNED16(class)
btConcaveShape : public btCollisionShape
{
protected:
	btScalar m_collisionMargin;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConcaveShape();

	virtual ~btConcaveShape();

	virtual void processAllTriangles(btTriangleCallback * callback, const btVector3& aabbMin, const btVector3& aabbMax) const = 0;

	virtual btScalar getMargin() const
	{
		return m_collisionMargin;
	}
	virtual void setMargin(btScalar collisionMargin)
	{
		m_collisionMargin = collisionMargin;
	}
};

#endif  
