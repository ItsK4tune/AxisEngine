

#ifndef BT_STATIC_PLANE_SHAPE_H
#define BT_STATIC_PLANE_SHAPE_H

#include "btConcaveShape.h"


ATTRIBUTE_ALIGNED16(class)
btStaticPlaneShape : public btConcaveShape
{
protected:
	btVector3 m_localAabbMin;
	btVector3 m_localAabbMax;

	btVector3 m_planeNormal;
	btScalar m_planeConstant;
	btVector3 m_localScaling;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btStaticPlaneShape(const btVector3& planeNormal, btScalar planeConstant);

	virtual ~btStaticPlaneShape();

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;

	virtual void processAllTriangles(btTriangleCallback * callback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual void setLocalScaling(const btVector3& scaling);
	virtual const btVector3& getLocalScaling() const;

	const btVector3& getPlaneNormal() const
	{
		return m_planeNormal;
	}

	const btScalar& getPlaneConstant() const
	{
		return m_planeConstant;
	}

	
	virtual const char* getName() const { return "STATICPLANE"; }

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};


struct btStaticPlaneShapeData
{
	btCollisionShapeData m_collisionShapeData;

	btVector3FloatData m_localScaling;
	btVector3FloatData m_planeNormal;
	float m_planeConstant;
	char m_pad[4];
};

SIMD_FORCE_INLINE int btStaticPlaneShape::calculateSerializeBufferSize() const
{
	return sizeof(btStaticPlaneShapeData);
}


SIMD_FORCE_INLINE const char* btStaticPlaneShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btStaticPlaneShapeData* planeData = (btStaticPlaneShapeData*)dataBuffer;
	btCollisionShape::serialize(&planeData->m_collisionShapeData, serializer);

	m_localScaling.serializeFloat(planeData->m_localScaling);
	m_planeNormal.serializeFloat(planeData->m_planeNormal);
	planeData->m_planeConstant = float(m_planeConstant);

	
	planeData->m_pad[0] = 0;
	planeData->m_pad[1] = 0;
	planeData->m_pad[2] = 0;
	planeData->m_pad[3] = 0;

	return "btStaticPlaneShapeData";
}

#endif  
