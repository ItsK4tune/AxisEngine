

#ifndef BT_CYLINDER_MINKOWSKI_H
#define BT_CYLINDER_MINKOWSKI_H

#include "btBoxShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  
#include "LinearMath/btVector3.h"


ATTRIBUTE_ALIGNED16(class)
btCylinderShape : public btConvexInternalShape

{
protected:
	int m_upAxis;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btVector3 getHalfExtentsWithMargin() const
	{
		btVector3 halfExtents = getHalfExtentsWithoutMargin();
		btVector3 margin(getMargin(), getMargin(), getMargin());
		halfExtents += margin;
		return halfExtents;
	}

	const btVector3& getHalfExtentsWithoutMargin() const
	{
		return m_implicitShapeDimensions;  
	}

	btCylinderShape(const btVector3& halfExtents);

	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	virtual void setMargin(btScalar collisionMargin)
	{
		
		btVector3 oldMargin(getMargin(), getMargin(), getMargin());
		btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions + oldMargin;

		btConvexInternalShape::setMargin(collisionMargin);
		btVector3 newMargin(getMargin(), getMargin(), getMargin());
		m_implicitShapeDimensions = implicitShapeDimensionsWithMargin - newMargin;
	}

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const
	{
		btVector3 supVertex;
		supVertex = localGetSupportingVertexWithoutMargin(vec);

		if (getMargin() != btScalar(0.))
		{
			btVector3 vecnorm = vec;
			if (vecnorm.length2() < (SIMD_EPSILON * SIMD_EPSILON))
			{
				vecnorm.setValue(btScalar(-1.), btScalar(-1.), btScalar(-1.));
			}
			vecnorm.normalize();
			supVertex += getMargin() * vecnorm;
		}
		return supVertex;
	}

	
	

	int getUpAxis() const
	{
		return m_upAxis;
	}

	virtual btVector3 getAnisotropicRollingFrictionDirection() const
	{
		btVector3 aniDir(0, 0, 0);
		aniDir[getUpAxis()] = 1;
		return aniDir;
	}

	virtual btScalar getRadius() const
	{
		return getHalfExtentsWithMargin().getX();
	}

	virtual void setLocalScaling(const btVector3& scaling)
	{
		btVector3 oldMargin(getMargin(), getMargin(), getMargin());
		btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions + oldMargin;
		btVector3 unScaledImplicitShapeDimensionsWithMargin = implicitShapeDimensionsWithMargin / m_localScaling;

		btConvexInternalShape::setLocalScaling(scaling);

		m_implicitShapeDimensions = (unScaledImplicitShapeDimensionsWithMargin * m_localScaling) - oldMargin;
	}

	
	virtual const char* getName() const
	{
		return "CylinderY";
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};

class btCylinderShapeX : public btCylinderShape
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btCylinderShapeX(const btVector3& halfExtents);

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	
	virtual const char* getName() const
	{
		return "CylinderX";
	}

	virtual btScalar getRadius() const
	{
		return getHalfExtentsWithMargin().getY();
	}
};

class btCylinderShapeZ : public btCylinderShape
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btCylinderShapeZ(const btVector3& halfExtents);

	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	
	virtual const char* getName() const
	{
		return "CylinderZ";
	}

	virtual btScalar getRadius() const
	{
		return getHalfExtentsWithMargin().getX();
	}
};


struct btCylinderShapeData
{
	btConvexInternalShapeData m_convexInternalShapeData;

	int m_upAxis;

	char m_padding[4];
};

SIMD_FORCE_INLINE int btCylinderShape::calculateSerializeBufferSize() const
{
	return sizeof(btCylinderShapeData);
}


SIMD_FORCE_INLINE const char* btCylinderShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btCylinderShapeData* shapeData = (btCylinderShapeData*)dataBuffer;

	btConvexInternalShape::serialize(&shapeData->m_convexInternalShapeData, serializer);

	shapeData->m_upAxis = m_upAxis;

	
	shapeData->m_padding[0] = 0;
	shapeData->m_padding[1] = 0;
	shapeData->m_padding[2] = 0;
	shapeData->m_padding[3] = 0;

	return "btCylinderShapeData";
}

#endif  
