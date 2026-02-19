

#ifndef BT_CONE_MINKOWSKI_H
#define BT_CONE_MINKOWSKI_H

#include "btConvexInternalShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  


ATTRIBUTE_ALIGNED16(class)
btConeShape : public btConvexInternalShape

{
	btScalar m_sinAngle;
	btScalar m_radius;
	btScalar m_height;
	int m_coneIndices[3];
	btVector3 coneLocalSupport(const btVector3& v) const;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConeShape(btScalar radius, btScalar height);

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	btScalar getRadius() const { return m_radius; }
	btScalar getHeight() const { return m_height; }

	void setRadius(const btScalar radius)
	{
		m_radius = radius;
	}
	void setHeight(const btScalar height)
	{
		m_height = height;
	}

	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const
	{
		btTransform identity;
		identity.setIdentity();
		btVector3 aabbMin, aabbMax;
		getAabb(identity, aabbMin, aabbMax);

		btVector3 halfExtents = (aabbMax - aabbMin) * btScalar(0.5);

		btScalar margin = getMargin();

		btScalar lx = btScalar(2.) * (halfExtents.x() + margin);
		btScalar ly = btScalar(2.) * (halfExtents.y() + margin);
		btScalar lz = btScalar(2.) * (halfExtents.z() + margin);
		const btScalar x2 = lx * lx;
		const btScalar y2 = ly * ly;
		const btScalar z2 = lz * lz;
		const btScalar scaledmass = mass * btScalar(0.08333333);

		inertia = scaledmass * (btVector3(y2 + z2, x2 + z2, x2 + y2));

		
		
		
	}

	virtual const char* getName() const
	{
		return "Cone";
	}

	
	void setConeUpIndex(int upIndex);

	int getConeUpIndex() const
	{
		return m_coneIndices[1];
	}

	virtual btVector3 getAnisotropicRollingFrictionDirection() const
	{
		return btVector3(0, 1, 0);
	}

	virtual void setLocalScaling(const btVector3& scaling);

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};


class btConeShapeX : public btConeShape
{
public:
	btConeShapeX(btScalar radius, btScalar height);

	virtual btVector3 getAnisotropicRollingFrictionDirection() const
	{
		return btVector3(1, 0, 0);
	}

	
	virtual const char* getName() const
	{
		return "ConeX";
	}
};


class btConeShapeZ : public btConeShape
{
public:
	btConeShapeZ(btScalar radius, btScalar height);

	virtual btVector3 getAnisotropicRollingFrictionDirection() const
	{
		return btVector3(0, 0, 1);
	}

	
	virtual const char* getName() const
	{
		return "ConeZ";
	}
};


struct btConeShapeData
{
	btConvexInternalShapeData m_convexInternalShapeData;

	int m_upIndex;

	char m_padding[4];
};

SIMD_FORCE_INLINE int btConeShape::calculateSerializeBufferSize() const
{
	return sizeof(btConeShapeData);
}


SIMD_FORCE_INLINE const char* btConeShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btConeShapeData* shapeData = (btConeShapeData*)dataBuffer;

	btConvexInternalShape::serialize(&shapeData->m_convexInternalShapeData, serializer);

	shapeData->m_upIndex = m_coneIndices[1];

	
	shapeData->m_padding[0] = 0;
	shapeData->m_padding[1] = 0;
	shapeData->m_padding[2] = 0;
	shapeData->m_padding[3] = 0;

	return "btConeShapeData";
}

#endif  
