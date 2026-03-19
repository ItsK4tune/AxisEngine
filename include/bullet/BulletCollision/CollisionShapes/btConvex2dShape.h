

#ifndef BT_CONVEX_2D_SHAPE_H
#define BT_CONVEX_2D_SHAPE_H

#include "BulletCollision/CollisionShapes/btConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  



ATTRIBUTE_ALIGNED16(class)
btConvex2dShape : public btConvexShape
{
	btConvexShape* m_childConvexShape;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConvex2dShape(btConvexShape * convexChildShape);

	virtual ~btConvex2dShape();

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	btConvexShape* getChildShape()
	{
		return m_childConvexShape;
	}

	const btConvexShape* getChildShape() const
	{
		return m_childConvexShape;
	}

	virtual const char* getName() const
	{
		return "Convex2dShape";
	}

	

	
	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void getAabbSlow(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void setLocalScaling(const btVector3& scaling);
	virtual const btVector3& getLocalScaling() const;

	virtual void setMargin(btScalar margin);
	virtual btScalar getMargin() const;

	virtual int getNumPreferredPenetrationDirections() const;

	virtual void getPreferredPenetrationDirection(int index, btVector3& penetrationVector) const;
};

#endif  
