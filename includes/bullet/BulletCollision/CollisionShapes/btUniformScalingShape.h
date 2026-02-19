

#ifndef BT_UNIFORM_SCALING_SHAPE_H
#define BT_UNIFORM_SCALING_SHAPE_H

#include "btConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  



ATTRIBUTE_ALIGNED16(class)
btUniformScalingShape : public btConvexShape
{
	btConvexShape* m_childConvexShape;

	btScalar m_uniformScalingFactor;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btUniformScalingShape(btConvexShape * convexChildShape, btScalar uniformScalingFactor);

	virtual ~btUniformScalingShape();

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	btScalar getUniformScalingFactor() const
	{
		return m_uniformScalingFactor;
	}

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
		return "UniformScalingShape";
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
