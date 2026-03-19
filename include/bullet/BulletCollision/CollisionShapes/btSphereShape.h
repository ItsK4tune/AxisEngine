
#ifndef BT_SPHERE_MINKOWSKI_H
#define BT_SPHERE_MINKOWSKI_H

#include "btConvexInternalShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  


ATTRIBUTE_ALIGNED16(class)
btSphereShape : public btConvexInternalShape

{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSphereShape(btScalar radius) : btConvexInternalShape()
	{
		m_shapeType = SPHERE_SHAPE_PROXYTYPE;
		m_localScaling.setValue(1.0, 1.0, 1.0);
		m_implicitShapeDimensions.setZero();
		m_implicitShapeDimensions.setX(radius);
		m_collisionMargin = radius;
		m_padding = 0;
	}

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	btScalar getRadius() const { return m_implicitShapeDimensions.getX() * m_localScaling.getX(); }

	void setUnscaledRadius(btScalar radius)
	{
		m_implicitShapeDimensions.setX(radius);
		btConvexInternalShape::setMargin(radius);
	}

	
	virtual const char* getName() const { return "SPHERE"; }

	virtual void setMargin(btScalar margin)
	{
		btConvexInternalShape::setMargin(margin);
	}
	virtual btScalar getMargin() const
	{
		
		
		return getRadius();
	}
};

#endif  
