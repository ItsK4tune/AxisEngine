
#ifndef BT_CONVEX_TRIANGLEMESH_SHAPE_H
#define BT_CONVEX_TRIANGLEMESH_SHAPE_H

#include "btPolyhedralConvexShape.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"  



ATTRIBUTE_ALIGNED16(class)
btConvexTriangleMeshShape : public btPolyhedralConvexAabbCachingShape
{
	class btStridingMeshInterface* m_stridingMesh;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btConvexTriangleMeshShape(btStridingMeshInterface * meshInterface, bool calcAabb = true);

	class btStridingMeshInterface* getMeshInterface()
	{
		return m_stridingMesh;
	}
	const class btStridingMeshInterface* getMeshInterface() const
	{
		return m_stridingMesh;
	}

	virtual btVector3 localGetSupportingVertex(const btVector3& vec) const;
	virtual btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const;
	virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors, btVector3* supportVerticesOut, int numVectors) const;

	
	virtual const char* getName() const { return "ConvexTrimesh"; }

	virtual int getNumVertices() const;
	virtual int getNumEdges() const;
	virtual void getEdge(int i, btVector3& pa, btVector3& pb) const;
	virtual void getVertex(int i, btVector3& vtx) const;
	virtual int getNumPlanes() const;
	virtual void getPlane(btVector3 & planeNormal, btVector3 & planeSupport, int i) const;
	virtual bool isInside(const btVector3& pt, btScalar tolerance) const;

	virtual void setLocalScaling(const btVector3& scaling);
	virtual const btVector3& getLocalScaling() const;

	
	
	
	
	
	void calculatePrincipalAxisTransform(btTransform & principal, btVector3 & inertia, btScalar & volume) const;
};

#endif  
