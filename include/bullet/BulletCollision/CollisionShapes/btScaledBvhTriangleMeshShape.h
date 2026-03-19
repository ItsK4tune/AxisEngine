

#ifndef BT_SCALED_BVH_TRIANGLE_MESH_SHAPE_H
#define BT_SCALED_BVH_TRIANGLE_MESH_SHAPE_H

#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"



ATTRIBUTE_ALIGNED16(class)
btScaledBvhTriangleMeshShape : public btConcaveShape
{
	btVector3 m_localScaling;

	btBvhTriangleMeshShape* m_bvhTriMeshShape;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btScaledBvhTriangleMeshShape(btBvhTriangleMeshShape * childShape, const btVector3& localScaling);

	virtual ~btScaledBvhTriangleMeshShape();

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;
	virtual void setLocalScaling(const btVector3& scaling);
	virtual const btVector3& getLocalScaling() const;
	virtual void calculateLocalInertia(btScalar mass, btVector3 & inertia) const;

	virtual void processAllTriangles(btTriangleCallback * callback, const btVector3& aabbMin, const btVector3& aabbMax) const;

	btBvhTriangleMeshShape* getChildShape()
	{
		return m_bvhTriMeshShape;
	}

	const btBvhTriangleMeshShape* getChildShape() const
	{
		return m_bvhTriMeshShape;
	}

	
	virtual const char* getName() const { return "SCALEDBVHTRIANGLEMESH"; }

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, btSerializer* serializer) const;
};


struct btScaledTriangleMeshShapeData
{
	btTriangleMeshShapeData m_trimeshShapeData;

	btVector3FloatData m_localScaling;
};

SIMD_FORCE_INLINE int btScaledBvhTriangleMeshShape::calculateSerializeBufferSize() const
{
	return sizeof(btScaledTriangleMeshShapeData);
}


SIMD_FORCE_INLINE const char* btScaledBvhTriangleMeshShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btScaledTriangleMeshShapeData* scaledMeshData = (btScaledTriangleMeshShapeData*)dataBuffer;
	m_bvhTriMeshShape->serialize(&scaledMeshData->m_trimeshShapeData, serializer);
	scaledMeshData->m_trimeshShapeData.m_collisionShapeData.m_shapeType = SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE;
	m_localScaling.serializeFloat(scaledMeshData->m_localScaling);
	return "btScaledTriangleMeshShapeData";
}

#endif  
