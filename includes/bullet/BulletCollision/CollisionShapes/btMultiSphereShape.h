

#ifndef BT_MULTI_SPHERE_MINKOWSKI_H
#define BT_MULTI_SPHERE_MINKOWSKI_H

#include "btConvexInternalShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  
#include "LinearMath/btAlignedObjectArray.h"
#include "LinearMath/btAabbUtil2.h"



ATTRIBUTE_ALIGNED16(class)
btMultiSphereShape : public btConvexInternalAabbCachingShape
{
	btAlignedObjectArray<btVector3> m_localPositionArray;
	btAlignedObjectArray<btScalar> m_radiArray;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMultiSphereShape(const btVector3* positions, const btScalar* radi, int numSpheres);

	
	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;

	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	int getSphereCount() const
	{
		return m_localPositionArray.size();
	}

	const btVector3& getSpherePosition(int index) const
	{
		return m_localPositionArray[index];
	}

	btScalar getSphereRadius(int index) const
	{
		return m_radiArray[index];
	}

	virtual const char* getName() const
	{
		return "MultiSphere";
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};

struct btPositionAndRadius
{
	btVector3FloatData m_pos;
	float m_radius;
};



struct	btMultiSphereShapeData
{
	btConvexInternalShapeData	m_convexInternalShapeData;

	btPositionAndRadius	*m_localPositionArrayPtr;
	int				m_localPositionArraySize;
	char	m_padding[4];
};



SIMD_FORCE_INLINE int btMultiSphereShape::calculateSerializeBufferSize() const
{
	return sizeof(btMultiSphereShapeData);
}

#endif  
