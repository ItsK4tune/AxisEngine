

#ifndef BT_CAPSULE_SHAPE_H
#define BT_CAPSULE_SHAPE_H

#include "btConvexInternalShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  




ATTRIBUTE_ALIGNED16(class)
btCapsuleShape : public btConvexInternalShape
{
protected:
	int m_upAxis;

protected:
	
	btCapsuleShape() : btConvexInternalShape() { m_shapeType = CAPSULE_SHAPE_PROXYTYPE; };

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btCapsuleShape(btScalar radius, btScalar height);

	
	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void setMargin(btScalar collisionMargin)
	{
		
		(void)collisionMargin;
	}

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		btVector3 halfExtents(getRadius(), getRadius(), getRadius());
		halfExtents[m_upAxis] = getRadius() + getHalfHeight();
		btMatrix3x3 abs_b = t.getBasis().absolute();
		btVector3 center = t.getOrigin();
		btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);

		aabbMin = center - extent;
		aabbMax = center + extent;
	}

	virtual const char* getName() const
	{
		return "CapsuleShape";
	}

	int getUpAxis() const
	{
		return m_upAxis;
	}

	btScalar getRadius() const
	{
		int radiusAxis = (m_upAxis + 2) % 3;
		return m_implicitShapeDimensions[radiusAxis];
	}

	btScalar getHalfHeight() const
	{
		return m_implicitShapeDimensions[m_upAxis];
	}

	virtual void setLocalScaling(const btVector3& scaling)
	{
		btVector3 unScaledImplicitShapeDimensions = m_implicitShapeDimensions / m_localScaling;
		btConvexInternalShape::setLocalScaling(scaling);
		m_implicitShapeDimensions = (unScaledImplicitShapeDimensions * scaling);
		
		int radiusAxis = (m_upAxis + 2) % 3;
		m_collisionMargin = m_implicitShapeDimensions[radiusAxis];
	}

	virtual btVector3 getAnisotropicRollingFrictionDirection() const
	{
		btVector3 aniDir(0, 0, 0);
		aniDir[getUpAxis()] = 1;
		return aniDir;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;

	SIMD_FORCE_INLINE void deSerializeFloat(struct btCapsuleShapeData * dataBuffer);
};



class btCapsuleShapeX : public btCapsuleShape
{
public:
	btCapsuleShapeX(btScalar radius, btScalar height);

	
	virtual const char* getName() const
	{
		return "CapsuleX";
	}
};



class btCapsuleShapeZ : public btCapsuleShape
{
public:
	btCapsuleShapeZ(btScalar radius, btScalar height);

	
	virtual const char* getName() const
	{
		return "CapsuleZ";
	}
};


struct btCapsuleShapeData
{
	btConvexInternalShapeData m_convexInternalShapeData;

	int m_upAxis;

	char m_padding[4];
};

SIMD_FORCE_INLINE int btCapsuleShape::calculateSerializeBufferSize() const
{
	return sizeof(btCapsuleShapeData);
}


SIMD_FORCE_INLINE const char* btCapsuleShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btCapsuleShapeData* shapeData = (btCapsuleShapeData*)dataBuffer;

	btConvexInternalShape::serialize(&shapeData->m_convexInternalShapeData, serializer);

	shapeData->m_upAxis = m_upAxis;

	
	shapeData->m_padding[0] = 0;
	shapeData->m_padding[1] = 0;
	shapeData->m_padding[2] = 0;
	shapeData->m_padding[3] = 0;

	return "btCapsuleShapeData";
}

SIMD_FORCE_INLINE void btCapsuleShape::deSerializeFloat(btCapsuleShapeData* dataBuffer)
{
	m_implicitShapeDimensions.deSerializeFloat(dataBuffer->m_convexInternalShapeData.m_implicitShapeDimensions);
	m_collisionMargin = dataBuffer->m_convexInternalShapeData.m_collisionMargin;
	m_localScaling.deSerializeFloat(dataBuffer->m_convexInternalShapeData.m_localScaling);
	
	m_upAxis = dataBuffer->m_upAxis;
}

#endif  
